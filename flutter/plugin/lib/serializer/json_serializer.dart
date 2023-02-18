import 'dart:convert';

import 'package:xplpc/message/message.dart';
import 'package:xplpc/message/param.dart';
import 'package:xplpc/serializer/serializer.dart';
import 'package:xplpc/util/log.dart';

class JsonSerializer implements Serializer {
  @override
  String decodeFunctionName(String data) {
    try {
      return json.decode(data)["f"];
    } catch (e) {
      Log.e("[JsonSerializer : decodeFunctionName] Error when parse json: $e");
    }

    return "";
  }

  @override
  T? decodeFunctionReturnValue<T>(String data) {
    try {
      return json.decode(data)["r"];
    } catch (e) {
      Log.e(
        "[JsonSerializer : decodeFunctionReturnValue] Error when parse json: $e",
      );
    }

    return null;
  }

  @override
  Message? decodeMessage(String data) {
    try {
      // decode parameters
      var decodedData = json.decode(data);

      // message data
      var message = Message();

      for (var param in decodedData["p"]) {
        message.set(param["n"], param["v"]);
      }

      return message;
    } catch (e) {
      Log.e("[JsonSerializer : decodeMessage] Error when decode message: $e");
    }

    return null;
  }

  @override
  String encodeFunctionReturnValue(data) {
    try {
      return jsonEncode({"r": data});
    } catch (e) {
      Log.e(
        "[JsonSerializer : encodeFunctionReturnValue] Error when encode data: $e",
      );
    }

    return "";
  }

  @override
  String encodeRequest(String functionName, [List<Param>? params]) {
    try {
      return json.encode({
        "f": functionName,
        "p": params,
      });
    } catch (e) {
      Log.e("[JsonSerializer : encodeRequest] Error when encode data: $e");
    }

    return "";
  }
}
