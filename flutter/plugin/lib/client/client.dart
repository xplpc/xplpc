import 'package:ffi/ffi.dart';
import 'package:xplpc/core/xplpc.dart';
import 'package:xplpc/data/callback_list.dart';
import 'package:xplpc/message/request.dart';
import 'package:xplpc/proxy/platform_proxy.dart';
import 'package:xplpc/type/typedefs.dart';
import 'package:xplpc/util/log.dart';
import 'package:xplpc/util/unique_id.dart';

class Client {
  static void call<T>(Request request, ClientCallback callback) {
    try {
      UniqueID.generate().then((key) {
        CallbackList.instance.add(key, (String response) {
          callback?.call(
            XPLPC.instance.config.serializer.decodeFunctionReturnValue<T>(
              response,
            ),
          );
        });

        var nativeKey = key.toNativeUtf8();
        var nativeData = request.data().toNativeUtf8();

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

  static void call(String data, ClientCallbackFromString callback) {
    try {
      UniqueID.generate().then((key) {
        CallbackList.instance.add(key, (String response) {
          callback?.call(response);
        });

        var nativeKey = key.toNativeUtf8();
        var nativeData = request.data().toNativeUtf8();

        PlatformProxy.nativeCallProxyFunc(
          nativeKey,
          nativeKey.length,
          nativeData,
          nativeData.length,
        );
      });
    } catch (e) {
      Log.e("[Client : call] Error: $e");
      callback?.call("");
    }
  }
}
