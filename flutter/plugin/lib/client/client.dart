import 'dart:async';

import 'package:xplpc/core/xplpc.dart';
import 'package:xplpc/data/callback_list.dart';
import 'package:xplpc/message/request.dart';
import 'package:xplpc/proxy/platform_proxy.dart';
import 'package:xplpc/type/typedefs.dart';
import 'package:xplpc/util/log.dart';
import 'package:xplpc/util/unique_id.dart';

class Client {
  static void call<T>(Request request, ClientCallback<T> callback) {
    final key = UniqueID.generate();

    try {
      CallbackList.instance.add(key, (String response) {
        callback?.call(
          XPLPC.instance.config.serializer.decodeFunctionReturnValue<T>(
            response,
          ),
        );
      });

      PlatformProxy.callNativeProxy(key, request.data());
    } catch (e) {
      Log.e("[Client : call] Error when reach the native side");
      Log.d("[Client : call] Error when reach the native side: $e");
      CallbackList.instance.remove(key);
      callback?.call(null);
    }
  }

  static void callFromString(
    String requestData,
    ClientCallbackFromString callback,
  ) {
    final key = UniqueID.generate();

    try {
      CallbackList.instance.add(key, (String response) {
        callback?.call(response);
      });

      PlatformProxy.callNativeProxy(key, requestData);
    } catch (e) {
      Log.e("[Client : callFromString] Error when reach the native side");
      Log.d("[Client : callFromString] Error when reach the native side: $e");
      CallbackList.instance.remove(key);
      callback?.call("");
    }
  }

  static T? callSync<T>(Request request) {
    final response = _answerSynchronously(request.data(), "callSync");

    if (response == null) {
      return null;
    }

    return XPLPC.instance.config.serializer.decodeFunctionReturnValue<T>(
      response,
    );
  }

  static String callSyncFromString(String requestData) {
    return _answerSynchronously(requestData, "callSyncFromString") ?? "";
  }

  // An isolate is single threaded, so nothing can write the answer while this function reads it.
  static String? _answerSynchronously(String data, String source) {
    String? answer;
    final key = UniqueID.generate();

    try {
      CallbackList.instance.add(key, (String response) {
        answer = response;
      });

      PlatformProxy.callNativeProxy(key, data);
    } catch (e) {
      Log.e("[Client : $source] Error when reach the native side");
      Log.d("[Client : $source] Error when reach the native side: $e");
      CallbackList.instance.remove(key);
      return null;
    }

    // Taking the key back is what decides the two cases, since a mapping that answered inline has already taken it and one that deferred never will.
    CallbackList.instance.remove(key);

    if (answer == null) {
      Log.e("[Client : $source] The function did not answer synchronously");
    }

    return answer;
  }

  static Future<T?> callAsync<T>(Request request) async {
    final completer = Completer<T?>();
    final key = UniqueID.generate();

    try {
      CallbackList.instance.add(key, (String response) {
        if (!completer.isCompleted) {
          completer.complete(
            XPLPC.instance.config.serializer.decodeFunctionReturnValue<T>(
              response,
            ),
          );
        }
      });

      PlatformProxy.callNativeProxy(key, request.data());
    } catch (e) {
      Log.e("[Client : callAsync] Error when reach the native side");
      Log.d("[Client : callAsync] Error when reach the native side: $e");
      CallbackList.instance.remove(key);

      if (!completer.isCompleted) {
        completer.complete(null);
      }
    }

    return completer.future;
  }

  static Future<String> callAsyncFromString(String requestData) async {
    final completer = Completer<String>();
    final key = UniqueID.generate();

    try {
      CallbackList.instance.add(key, (String response) {
        if (!completer.isCompleted) {
          completer.complete(response);
        }
      });

      PlatformProxy.callNativeProxy(key, requestData);
    } catch (e) {
      Log.e("[Client : callAsyncFromString] Error when reach the native side");
      Log.d(
        "[Client : callAsyncFromString] Error when reach the native side: $e",
      );
      CallbackList.instance.remove(key);

      if (!completer.isCompleted) {
        completer.complete("");
      }
    }

    return completer.future;
  }
}
