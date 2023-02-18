import 'package:xplpc/core/xplpc.dart';
import 'package:xplpc/data/mapping_list.dart';
import 'package:xplpc/message/message.dart';
import 'package:xplpc/type/typedefs.dart';
import 'package:xplpc/util/log.dart';

class ProxyClient {
  static void call(String data, ProxyClientCallback callback) {
    // function name
    var functionName = XPLPC.instance.config.serializer.decodeFunctionName(
      data,
    );

    if (functionName.isEmpty) {
      Log.e("[ProxyClient : call] Function name is empty");
      callback?.call("");
      return;
    }

    // mapping item
    var mappingItem = MappingList.instance.find(functionName);

    if (mappingItem == null) {
      Log.e(
        "[ProxyClient : call] Mapping not found for function: $functionName",
      );
      callback?.call("");
      return;
    }

    // execute
    Message? message;

    try {
      message = XPLPC.instance.config.serializer.decodeMessage(data);
    } catch (e) {
      Log.e("[ProxyClient : call] Error when decode message: $e");
    }

    if (message == null) {
      Log.e(
        "[ProxyClient : call] Error when decode message for function: $functionName",
      );
      callback?.call("");
      return;
    }

    try {
      mappingItem.target(message, (dynamic r) {
        callback
            ?.call(XPLPC.instance.config.serializer.encodeFunctionReturnValue(
          r,
        ));
      });

      return;
    } catch (e) {
      Log.e("[ProxyClient : call] Error: $e");
    }

    callback?.call("");
  }
}
