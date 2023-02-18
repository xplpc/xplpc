import 'package:ffi/ffi.dart';
import 'package:xplpc/core/xplpc.dart';
import 'package:xplpc/data/callback_list.dart';
import 'package:xplpc/message/request.dart';
import 'package:xplpc/proxy/platform_proxy.dart';
import 'package:xplpc/type/typedefs.dart';
import 'package:xplpc/util/log.dart';
import 'package:xplpc/util/unique_id.dart';

class RemoteClient {
  static void call<T>(Request request, RemoteClientCallback callback) {
    try {
      UniqueID.generate().then((key) {
        CallbackList.instance.add(key, (String response) {
          callback(XPLPC.instance.config.serializer
              .decodeFunctionReturnValue<T>(response));
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

      return;
    } on Exception catch (e) {
      Log.e("[RemoteClient : call] Error: $e");
    }

    callback(null);
  }
}
