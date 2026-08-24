import 'dart:ffi' as ffi;
import 'dart:io';

import "package:ffi/ffi.dart";
import 'package:xplpc/core/xplpc.dart';
import 'package:xplpc/data/callback_list.dart';
import 'package:xplpc/data/mapping_list.dart';
import 'package:xplpc/type/typedefs.dart';
import 'package:xplpc/util/log.dart';

class PlatformProxy {
  static late InitializeFunc initializeFunc;
  static late CallProxyFunc nativeCallProxyFunc;
  static late CallProxyCallbackFunc nativeCallProxyCallbackFunc;
  static late MappingFunc nativeAddMappingFunc;
  static late ClearMappingsFunc nativeClearMappingsFunc;
  static late FreeFunc nativeFreeFunc;

  static final onInitializePlatformFunc =
      ffi.Pointer.fromFunction<OnInitializePlatform>(onInitializePlatform);

  static final onFinalizePlatformFunc =
      ffi.Pointer.fromFunction<OnFinalizePlatform>(onFinalizePlatform);

  static final onNativeProxyCallFunc =
      ffi.Pointer.fromFunction<CallProxyCallback>(onNativeProxyCall);

  static final onNativeProxyCallbackFunc =
      ffi.Pointer.fromFunction<CallProxyCallback>(onNativeProxyCallback);

  // An answer produced on a thread the isolate does not run on can only be delivered through its event loop.
  static final onNativeProxyCallFromThreadCallable =
      ffi.NativeCallable<CallProxyCallback>.listener(
        onNativeProxyCallFromThread,
      )..keepIsolateAlive = false;

  static final onNativeProxyCallbackFromThreadCallable =
      ffi.NativeCallable<CallProxyCallback>.listener(
        onNativeProxyCallbackFromThread,
      )..keepIsolateAlive = false;

  static void initialize() {
    initializeFunc = XPLPC.instance.library
        .lookupFunction<NativeInitializeFunc, InitializeFunc>(
          'xplpc_core_initialize',
        );
    nativeCallProxyFunc = XPLPC.instance.library
        .lookupFunction<NativeCallProxyFunc, CallProxyFunc>(
          'xplpc_native_call_proxy',
        );
    nativeCallProxyCallbackFunc = XPLPC.instance.library
        .lookupFunction<NativeCallProxyCallbackFunc, CallProxyCallbackFunc>(
          'xplpc_native_call_proxy_callback',
        );
    nativeAddMappingFunc = XPLPC.instance.library
        .lookupFunction<NativeMappingFunc, MappingFunc>(
          'xplpc_native_add_mapping',
        );
    nativeClearMappingsFunc = XPLPC.instance.library
        .lookupFunction<NativeClearMappingsFunc, ClearMappingsFunc>(
          'xplpc_native_clear_mappings',
        );
    nativeFreeFunc = XPLPC.instance.library
        .lookupFunction<NativeFreeFunc, FreeFunc>('xplpc_free');

    var initializeCxxPlatformProxy = true;

    if (Platform.isAndroid) {
      initializeCxxPlatformProxy = false;
    } else if (Platform.isMacOS) {
      initializeCxxPlatformProxy = false;
    } else if (Platform.isIOS) {
      initializeCxxPlatformProxy = false;
    }

    initializeFunc(
      initializeCxxPlatformProxy,
      onInitializePlatformFunc,
      onFinalizePlatformFunc,
      onNativeProxyCallFunc,
      onNativeProxyCallbackFunc,
      onNativeProxyCallFromThreadCallable.nativeFunction,
      onNativeProxyCallbackFromThreadCallable.nativeFunction,
    );
  }

  static void onNativeProxyCall(
    ffi.Pointer<Utf8> key,
    int keySize,
    ffi.Pointer<Utf8> data,
    int dataSize,
  ) {
    final String keyStr;
    final String dataStr;

    try {
      keyStr = key.toDartString(length: keySize);
    } on FormatException catch (e) {
      // There is nothing to answer under a key that cannot be read, so the call is reported and dropped.
      Log.e("[PlatformProxy : call] Unable to decode key");
      Log.d("[PlatformProxy : call] Unable to decode key: $e");
      return;
    }

    try {
      dataStr = data.toDartString(length: dataSize);
    } on FormatException catch (e) {
      Log.e("[PlatformProxy : call] Unable to decode data");
      Log.d("[PlatformProxy : call] Unable to decode data: $e");
      callNativeProxyCallback(keyStr, "");
      return;
    }

    if (!XPLPC.instance.initialized) {
      Log.e("[PlatformProxy : call] XPLPC was not initialized");
      callNativeProxyCallback(keyStr, "");
      return;
    }

    final request = XPLPC.instance.config.serializer.decodeRequest(dataStr);

    if (request == null) {
      callNativeProxyCallback(keyStr, "");
      return;
    }

    if (request.functionName.isEmpty) {
      Log.e("[PlatformProxy : call] Function name is empty");
      callNativeProxyCallback(keyStr, "");
      return;
    }

    final mappingItem = MappingList.instance.find(request.functionName);

    if (mappingItem == null) {
      Log.e(
        "[PlatformProxy : call] Mapping not found for function: ${request.functionName}",
      );
      callNativeProxyCallback(keyStr, "");
      return;
    }

    try {
      mappingItem.target(request.message, (dynamic r) {
        final decodedData = XPLPC.instance.config.serializer
            .encodeFunctionReturnValue(r);

        callNativeProxyCallback(keyStr, decodedData);
      });
    } catch (e) {
      Log.e(
        '[PlatformProxy : call] Error when execute function "${request.functionName}"',
      );
      Log.d(
        '[PlatformProxy : call] Error when execute function "${request.functionName}": $e',
      );
      callNativeProxyCallback(keyStr, "");
    }
  }

  static void onNativeProxyCallback(
    ffi.Pointer<Utf8> key,
    int keySize,
    ffi.Pointer<Utf8> data,
    int dataSize,
  ) {
    final String keyStr;

    try {
      keyStr = key.toDartString(length: keySize);
    } on FormatException catch (e) {
      // There is nothing to resolve under a key that cannot be read.
      Log.e("[PlatformProxy : onNativeProxyCallback] Unable to decode key");
      Log.d("[PlatformProxy : onNativeProxyCallback] Unable to decode key: $e");
      return;
    }

    String dataStr;

    try {
      dataStr = data.toDartString(length: dataSize);
    } on FormatException catch (e) {
      Log.e("[PlatformProxy : onNativeProxyCallback] Unable to decode data");
      Log.d(
        "[PlatformProxy : onNativeProxyCallback] Unable to decode data: $e",
      );
      dataStr = "";
    }

    CallbackList.instance.execute(keyStr, dataStr);
  }

  static void onNativeProxyCallFromThread(
    ffi.Pointer<Utf8> key,
    int keySize,
    ffi.Pointer<Utf8> data,
    int dataSize,
  ) {
    // The buffers were copied for this isolate, so they are released once they have been read.

    try {
      onNativeProxyCall(key, keySize, data, dataSize);
    } finally {
      _releaseNativeStrings(key, data);
    }
  }

  static void onNativeProxyCallbackFromThread(
    ffi.Pointer<Utf8> key,
    int keySize,
    ffi.Pointer<Utf8> data,
    int dataSize,
  ) {
    try {
      onNativeProxyCallback(key, keySize, data, dataSize);
    } finally {
      _releaseNativeStrings(key, data);
    }
  }

  static void onInitializePlatform() {}

  static void onFinalizePlatform() {
    MappingList.instance.clear();
  }

  static void callNativeProxy(String key, String data) {
    _withNativeStrings(key, data, nativeCallProxyFunc);
  }

  static void addMapping(String name) {
    // The native side answers hasMapping from these names, so it never has to reach the isolate to resolve a function.

    final nativeName = name.toNativeUtf8();

    try {
      nativeAddMappingFunc(nativeName, nativeName.length);
    } finally {
      malloc.free(nativeName);
    }
  }

  static void clearMappings() {
    nativeClearMappingsFunc();
  }

  static void callNativeProxyCallback(String key, String data) {
    _withNativeStrings(key, data, nativeCallProxyCallbackFunc);
  }

  static void _withNativeStrings(
    String key,
    String data,
    void Function(ffi.Pointer<Utf8>, int, ffi.Pointer<Utf8>, int) body,
  ) {
    // The native side copies both buffers before returning, so they are released as soon as the call completes.

    final nativeKey = key.toNativeUtf8();
    final nativeData = data.toNativeUtf8();

    try {
      body(nativeKey, nativeKey.length, nativeData, nativeData.length);
    } finally {
      malloc.free(nativeKey);
      malloc.free(nativeData);
    }
  }

  static void _releaseNativeStrings(
    ffi.Pointer<Utf8> key,
    ffi.Pointer<Utf8> data,
  ) {
    nativeFreeFunc(key.cast());
    nativeFreeFunc(data.cast());
  }
}
