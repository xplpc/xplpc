import 'package:xplpc/core/xplpc.dart';
import 'package:xplpc/data/mapping_list.dart';
import 'package:xplpc/message/message.dart';
import 'package:xplpc/message/request.dart';
import 'package:xplpc/type/typedefs.dart';
import 'package:xplpc/util/log.dart';

class LocalClient {
  static void call<T>(Request request, LocalClientCallback<T?> callback) {
    var data = request.data();

    // function name
    var functionName = XPLPC.instance.config.serializer.decodeFunctionName(
      data,
    );

    if (functionName.isEmpty) {
      Log.e("[LocalClient : call] Function name is empty");
      callback?.call(null);
      return;
    }

    // mapping item
    var mappingItem = MappingList.instance.find(functionName);

    if (mappingItem == null) {
      Log.e(
        "[LocalClient : call] Mapping not found for function: $functionName",
      );
      callback?.call(null);
      return;
    }

    // execute
    Message? message;

    try {
      message = XPLPC.instance.config.serializer.decodeMessage(data);
    } catch (e) {
      Log.e("[LocalClient : call] Error when decode message: $e");
    }

    if (message == null) {
      Log.e(
        "[LocalClient : call] Error when decode message for function: $functionName",
      );
      callback?.call(null);
      return;
    }

    try {
      mappingItem.target(message, (dynamic r) {
        callback?.call(r as T?);
      });

      return;
    } catch (e) {
      Log.e("[LocalClient : call] Error: $e");
    }

    callback?.call(null);
  }
}
