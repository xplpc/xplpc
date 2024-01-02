import 'dart:async';

import 'package:ffi/ffi.dart';
import 'package:xplpc/core/xplpc.dart';
import 'package:xplpc/data/callback_list.dart';
import 'package:xplpc/message/request.dart';
import 'package:xplpc/proxy/platform_proxy.dart';
import 'package:xplpc/type/typedefs.dart';
import 'package:xplpc/util/log.dart';
import 'package:xplpc/util/unique_id.dart';

class Client {
  static void call<T>(
    Request request,
    ClientCallback callback,
  ) {
    try {
      UniqueID.generate().then((key) {
        CallbackList.instance.add(key, (String response) {
          callback?.call(
            XPLPC.instance.config.serializer.decodeFunctionReturnValue<T>(
              response,
            ),
          );
        });

        final nativeKey = key.toNativeUtf8();
        final nativeData = request.data().toNativeUtf8();

        PlatformProxy.nativeCallProxyFunc(
          nativeKey,
          nativeKey.length,
          nativeData,
          nativeData.length,
        );
      });
    } catch (e) {
      Log.e("[Client : call] Error: $e");
      callback?.call(null);
    }
  }

  static void callFromString(
    String requestData,
    ClientCallbackFromString callback,
  ) {
    try {
      UniqueID.generate().then((key) {
        CallbackList.instance.add(key, (String response) {
          callback?.call(response);
        });

        final nativeKey = key.toNativeUtf8();
        final nativeData = requestData.toNativeUtf8();

        PlatformProxy.nativeCallProxyFunc(
          nativeKey,
          nativeKey.length,
          nativeData,
          nativeData.length,
        );
      });
    } catch (e) {
      Log.e("[Client : callFromString] Error: $e");
      callback?.call("");
    }
  }

  static Future<T?> callAsync<T>(
    Request request,
  ) async {
    Completer<T?> completer = Completer();

    try {
      String key = await UniqueID.generate();

      CallbackList.instance.add(key, (String response) {
        if (!completer.isCompleted) {
          completer.complete(
            XPLPC.instance.config.serializer.decodeFunctionReturnValue<T>(
              response,
            ),
          );
        }
      });

      final nativeKey = key.toNativeUtf8();
      final nativeData = request.data().toNativeUtf8();

      PlatformProxy.nativeCallProxyFunc(
        nativeKey,
        nativeKey.length,
        nativeData,
        nativeData.length,
      );
    } catch (e) {
      Log.e("[Client : callAsync] Error: $e");

      if (!completer.isCompleted) {
        completer.complete(null);
      }
    }

    return completer.future;
  }

  static Future<String> callAsyncFromString(
    String requestData,
  ) async {
    Completer<String> completer = Completer();

    try {
      String key = await UniqueID.generate();

      CallbackList.instance.add(key, (String response) {
        if (!completer.isCompleted) {
          completer.complete(response);
        }
      });

      final nativeKey = key.toNativeUtf8();
      final nativeData = requestData.toNativeUtf8();

      PlatformProxy.nativeCallProxyFunc(
        nativeKey,
        nativeKey.length,
        nativeData,
        nativeData.length,
      );
    } catch (e) {
      Log.e("[Client : callAsyncFromString] Error: $e");

      if (!completer.isCompleted) {
        completer.completeError(e);
      }
    }

    return completer.future;
  }
}
