import 'dart:convert';
import 'dart:ffi' as ffi;
import 'dart:typed_data';

import "package:ffi/ffi.dart";
import 'package:xplpc/core/xplpc.dart';
import 'package:xplpc/data/mapping_list.dart';
import 'package:xplpc/data/pending_call.dart';
import 'package:xplpc/message/decoded_request.dart';
import 'package:xplpc/type/typedefs.dart';
import 'package:xplpc/util/log.dart';

// The isolate as one layer of the router, entered directly only while it runs a call it started and reached through its event loop otherwise.
class Layer {
  static const _postAnswer = 0;
  static const _postInvoke = 1;

  // An isolate that restarts in the same process attaches under this name again, which detaches the layer it left behind.
  static const _name = "dart";

  static late AttachFunc _attach;
  static late NameFunc _declare;
  static late NameFunc _retract;
  static late CallFunc _call;
  static late CancelFunc _cancel;
  static late AnswerFunc _answer;
  static late BufferRegisterFunc _bufferRegister;
  static late BufferReadFunc _bufferRead;
  static late BufferReleaseFunc _bufferRelease;
  static late PendingFunc _nativePending;
  static late FreeFunc _free;

  static late ffi.NativeCallable<InvokeCallback> _invokeCallable;
  static late ffi.NativeCallable<PostCallback> _postCallable;

  static int _handle = 0;

  // Every callable dies with the isolate, so the layer is abandoned by a finalizer the isolate runs as it shuts down rather than left for a late answer or a call to reach.
  static late ffi.NativeFinalizer _abandonment;
  static final _lifetime = _Lifetime();

  // Dart cannot hand the native side an object it would hand back, so a call is known by an integer that is unique among the calls this isolate is waiting on.
  static int _nextToken = 0;
  static final _pending = <int, void Function(String)>{};

  static final _buffers = <int>{};

  static void initialize(ffi.DynamicLibrary library) {
    _attach = library.lookupFunction<NativeAttachFunc, AttachFunc>(
      'xplpc_attach',
    );
    _declare = library.lookupFunction<NativeNameFunc, NameFunc>(
      'xplpc_declare',
    );
    _retract = library.lookupFunction<NativeNameFunc, NameFunc>(
      'xplpc_retract',
    );
    _call = library.lookupFunction<NativeCallFunc, CallFunc>('xplpc_call');
    _cancel = library.lookupFunction<NativeCancelFunc, CancelFunc>(
      'xplpc_cancel',
    );
    _answer = library.lookupFunction<NativeAnswerFunc, AnswerFunc>(
      'xplpc_answer',
    );
    _bufferRegister = library
        .lookupFunction<NativeBufferRegisterFunc, BufferRegisterFunc>(
          'xplpc_buffer_register',
        );
    _bufferRead = library.lookupFunction<NativeBufferReadFunc, BufferReadFunc>(
      'xplpc_buffer_read',
    );
    _bufferRelease = library
        .lookupFunction<NativeBufferReleaseFunc, BufferReleaseFunc>(
          'xplpc_buffer_release',
        );
    _nativePending = library.lookupFunction<NativePendingFunc, PendingFunc>(
      'xplpc_pending',
    );
    _free = library.lookupFunction<NativeFreeFunc, FreeFunc>('xplpc_free');

    final abandon = library.lookup<ffi.NativeFinalizerFunction>(
      'xplpc_abandon',
    );

    _invokeCallable = ffi.NativeCallable<InvokeCallback>.isolateLocal(_onInvoke)
      ..keepIsolateAlive = false;

    _postCallable = ffi.NativeCallable<PostCallback>.listener(_onPost)
      ..keepIsolateAlive = false;

    final host = calloc<NativeHost>();
    final name = _name.toNativeUtf8();

    try {
      host.ref.invoke = _invokeCallable.nativeFunction;
      host.ref.post = _postCallable.nativeFunction;
      host.ref.enterableAnywhere = false;
      host.ref.name = name;
      host.ref.nameSize = name.length;

      _handle = _attach(host);
    } finally {
      calloc.free(host);
      malloc.free(name);
    }

    if (_handle == 0) {
      _invokeCallable.close();
      _postCallable.close();

      throw StateError("XPLPC could not attach the dart layer");
    }

    // The finalizer reads the handle after the isolate is gone, so it lives in memory the isolate never releases.
    final handle = calloc<ffi.Uint64>()..value = _handle;

    _abandonment = ffi.NativeFinalizer(abandon);

    _abandonment.attach(_lifetime, handle.cast());
  }

  static void declare(String name) {
    _withNativeString(name, (pointer, length) {
      _declare(_handle, pointer, length);
    });
  }

  static void retract(String name) {
    _withNativeString(name, (pointer, length) {
      _retract(_handle, pointer, length);
    });
  }

  // A mapping that answered before the call returned is answered in the pending call itself, and the callback only receives an answer that arrives later.
  static PendingCall call(String data, void Function(String) callback) {
    final token = ++_nextToken;

    return _withNativeString(data, (request, length) {
      final result = calloc<NativeCallResult>();

      try {
        if (_call(_handle, token, request, length, result)) {
          return PendingCall.answered(
            _readOwned(result.ref.data, result.ref.dataSize),
          );
        }

        // A later answer reaches the isolate through its event loop, which cannot run before this returns.
        _pending[token] = callback;

        return PendingCall.waiting(token, result.ref.call);
      } finally {
        calloc.free(result);
      }
    });
  }

  // Taking the call back is what decides between the answer and the cancel, since whichever loses resolves nothing.
  static bool cancel(PendingCall pending) {
    if (pending.answered || !_cancel(_handle, pending.call)) {
      return false;
    }

    _pending.remove(pending.token);

    return true;
  }

  // The calls still waiting in the process and in this isolate, which is what lets a suite prove it left none behind.
  static int get pending => _nativePending() + _pending.length;

  // The native side reads the bytes in place until the handle is released, so the caller keeps them valid and unmoved until then.
  static int registerBuffer(ffi.Pointer<ffi.Uint8> data, int size) {
    final buffer = _bufferRegister(_handle, data.cast(), size);

    if (buffer != 0) {
      _buffers.add(buffer);
    }

    return buffer;
  }

  // Only the layer that registered a buffer can release it, so a handle this isolate received or already released answers false.
  static bool releaseBuffer(int buffer) {
    // A view naming nothing is what a failed registration answers, and that failure was already reported when it happened.
    if (buffer == 0) {
      return false;
    }

    if (!_bufferRelease(_handle, buffer)) {
      return false;
    }

    _buffers.remove(buffer);

    return true;
  }

  // A handle naming nothing, released or never registered reads as no bytes, and what is read is a copy, since the owner may release the bytes right afterwards.
  static Uint8List readBuffer(int buffer) {
    if (buffer == 0) {
      return Uint8List(0);
    }

    final data = calloc<ffi.Pointer<ffi.Void>>();
    final size = calloc<ffi.Size>();

    try {
      if (!_bufferRead(buffer, data, size)) {
        return Uint8List(0);
      }

      return Uint8List.fromList(
        data.value.cast<ffi.Uint8>().asTypedList(size.value),
      );
    } finally {
      calloc.free(data);
      calloc.free(size);
    }
  }

  // The buffers this isolate registered and has not released yet, which is what lets a suite prove it left none behind.
  static int get buffers => _buffers.length;

  static void _onInvoke(
    ffi.Pointer<ffi.Void> context,
    int call,
    ffi.Pointer<Utf8> data,
    int dataSize,
  ) {
    final String request;

    try {
      request = data.toDartString(length: dataSize);
    } on FormatException catch (e) {
      Log.e("[Layer : invoke] Unable to decode data");
      Log.d("[Layer : invoke] Unable to decode data: $e");
      _respond(call, "");
      return;
    }

    _serve(call, request);
  }

  static void _onPost(
    ffi.Pointer<ffi.Void> context,
    int kind,
    int id,
    ffi.Pointer<Utf8> data,
    int dataSize,
  ) {
    final text = _readOwned(data, dataSize);

    // The listener is the frame an answer arrives in, so a callback that raises there is reported rather than left to end the isolate.
    if (kind == _postAnswer) {
      try {
        _pending.remove(id)?.call(text);
      } catch (e) {
        Log.e("[Layer : answer] The callback raised while being answered");
        Log.d("[Layer : answer] The callback raised while being answered: $e");
      }

      return;
    }

    if (kind == _postInvoke) {
      _serve(id, text);
    }
  }

  static void _serve(int call, String data) {
    if (!XPLPC.instance.initialized) {
      Log.e("[Layer : invoke] XPLPC was not initialized");
      _respond(call, "");
      return;
    }

    // A serializer is code this library may not have written, and the call it fails to read is answered rather than left waiting.
    final DecodedRequest? request;

    try {
      request = XPLPC.instance.config.serializer.decodeRequest(data);
    } catch (e) {
      Log.e("[Layer : invoke] Error when decode request");
      Log.d("[Layer : invoke] Error when decode request: $e");
      _respond(call, "");
      return;
    }

    if (request == null) {
      _respond(call, "");
      return;
    }

    final functionName = request.functionName;
    final mappingItem = MappingList.instance.find(functionName);

    if (mappingItem == null) {
      Log.e("[Layer : invoke] Mapping not found for function: $functionName");
      _respond(call, "");
      return;
    }

    try {
      mappingItem.target(request.message, (dynamic r) {
        _respond(call, _encode(functionName, r));
      });
    } catch (e) {
      Log.e('[Layer : invoke] Error when execute function "$functionName"');
      Log.d('[Layer : invoke] Error when execute function "$functionName": $e');
      _respond(call, "");
    }
  }

  // A mapping may answer from a later turn of the event loop, where nothing above this can catch what a serializer raises.
  static String _encode(String functionName, dynamic value) {
    try {
      return XPLPC.instance.config.serializer.encodeFunctionReturnValue(value);
    } catch (e) {
      Log.e(
        '[Layer : invoke] Error when encode the answer of function "$functionName"',
      );
      Log.d(
        '[Layer : invoke] Error when encode the answer of function "$functionName": $e',
      );

      return "";
    }
  }

  static void _respond(int call, String response) {
    _withNativeString(response, (pointer, length) {
      _answer(call, pointer, length);
    });
  }

  // The native side owns nothing it handed over, so the bytes are read and released here.
  static String _readOwned(ffi.Pointer<Utf8> data, int dataSize) {
    if (data == ffi.nullptr) {
      return "";
    }

    try {
      return data.toDartString(length: dataSize);
    } on FormatException catch (e) {
      Log.e("[Layer : answer] Unable to decode data");
      Log.d("[Layer : answer] Unable to decode data: $e");
      return "";
    } finally {
      _free(data.cast());
    }
  }

  static T _withNativeString<T>(
    String value,
    T Function(ffi.Pointer<Utf8>, int) body,
  ) {
    // The length is the count of bytes encoded rather than what strlen finds, since a string may carry a nul the native side has to see.
    // The native side copies the buffer before returning, so it is released as soon as the call completes.

    final bytes = utf8.encode(value);
    final pointer = malloc<ffi.Uint8>(bytes.isEmpty ? 1 : bytes.length);

    try {
      pointer.asTypedList(bytes.length).setAll(0, bytes);

      return body(pointer.cast<Utf8>(), bytes.length);
    } finally {
      malloc.free(pointer);
    }
  }
}

// Lives as long as the isolate does, which is what the finalizer that abandons the layer is attached to.
final class _Lifetime implements ffi.Finalizable {}
