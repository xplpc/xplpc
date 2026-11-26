import 'dart:async';

import 'package:xplpc/core/xplpc.dart';
import 'package:xplpc/data/pending_call.dart';
import 'package:xplpc/layer/layer.dart';
import 'package:xplpc/message/request.dart';
import 'package:xplpc/type/typedefs.dart';
import 'package:xplpc/util/log.dart';

class Client {
  static void call<T>(Request request, ClientCallback<T> callback) {
    if (!_ready("call")) {
      callback?.call(null);
      return;
    }

    _dispatch(_data(request, "call"), "call", (String response) {
      callback?.call(_decode<T>(response, "call"));
    });
  }

  static void callFromString(
    String requestData,
    ClientCallbackFromString callback,
  ) {
    if (!_ready("callFromString")) {
      callback?.call("");
      return;
    }

    _dispatch(requestData, "callFromString", (String response) {
      callback?.call(response);
    });
  }

  static T? callSync<T>(Request request) {
    if (!_ready("callSync")) {
      return null;
    }

    final response = _answerSynchronously(
      _data(request, "callSync"),
      "callSync",
    );

    if (response == null) {
      return null;
    }

    return _decode<T>(response, "callSync");
  }

  static String callSyncFromString(String requestData) {
    if (!_ready("callSyncFromString")) {
      return "";
    }

    return _answerSynchronously(requestData, "callSyncFromString") ?? "";
  }

  static Future<T?> callAsync<T>(Request request) async {
    if (!_ready("callAsync")) {
      return null;
    }

    final completer = Completer<T?>();

    _dispatch(_data(request, "callAsync"), "callAsync", (String response) {
      completer.complete(_decode<T>(response, "callAsync"));
    });

    return completer.future;
  }

  static Future<String> callAsyncFromString(String requestData) async {
    if (!_ready("callAsyncFromString")) {
      return "";
    }

    final completer = Completer<String>();

    _dispatch(requestData, "callAsyncFromString", completer.complete);

    return completer.future;
  }

  // A request is written with the config and reaches a native side the library only holds once it is up.
  static bool _ready(String source) {
    if (XPLPC.instance.initialized) {
      return true;
    }

    Log.e("[Client : $source] XPLPC was not initialized");

    return false;
  }

  // A serializer of the caller's own can raise while it writes the request, and the caller is answered rather than handed the exception.
  static String _data(Request request, String source) {
    try {
      return request.data();
    } catch (e) {
      Log.e("[Client : $source] Error when encode the request");
      Log.d("[Client : $source] Error when encode the request: $e");
    }

    return "";
  }

  // Only the decode is guarded, so a serializer that raises still answers the caller and a callback that raises on a valid answer is never called a second time.
  static T? _decode<T>(String response, String source) {
    try {
      return XPLPC.instance.config.serializer.decodeFunctionReturnValue<T>(
        response,
      );
    } catch (e) {
      Log.e("[Client : $source] Error when decode data");
      Log.d("[Client : $source] Error when decode data: $e");
    }

    return null;
  }

  // A mapping that had not answered when the call returned is cancelled, so its later answer resolves nothing and leaves nothing behind.
  static String? _answerSynchronously(String data, String source) {
    String? answer;

    final pending = _dispatch(data, source, (String response) {
      answer = response;
    });

    if (pending == null || pending.answered) {
      return answer;
    }

    Layer.cancel(pending);
    Log.e("[Client : $source] The function did not answer synchronously");

    return null;
  }

  // The callback runs outside the guard, since what it raises belongs to the caller and is not a native side that could not be reached.
  static PendingCall? _dispatch(
    String data,
    String source,
    void Function(String) callback,
  ) {
    final PendingCall pending;

    try {
      pending = Layer.call(data, callback);
    } catch (e) {
      Log.e("[Client : $source] Error when reach the native side");
      Log.d("[Client : $source] Error when reach the native side: $e");
      callback("");

      return null;
    }

    final answer = pending.answer;

    if (answer != null) {
      callback(answer);
    }

    return pending;
  }
}
