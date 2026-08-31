import 'dart:convert';

import 'package:reflectable/reflectable.dart';
import 'package:xplpc/message/decoded_request.dart';
import 'package:xplpc/message/message.dart';
import 'package:xplpc/message/param.dart';
import 'package:xplpc/reflectable/reflector.dart';
import 'package:xplpc/serializer/serializer.dart';
import 'package:xplpc/util/log.dart';

class JsonSerializer implements Serializer {
  @override
  T? decodeFunctionReturnValue<T>(String data) {
    // An empty response is the empty value every failing path answers with, not a document that failed to parse.
    if (data.isEmpty) {
      return null;
    }

    // A reflected type is filled through the fromJson constructor it declares.
    try {
      ClassMirror? classMirror = reflector.reflectType(T) as ClassMirror?;

      if (classMirror != null &&
          classMirror.declarations.values.whereType<MethodMirror>().any(
            (m) =>
                m.simpleName.toString().substring(
                  m.simpleName.toString().lastIndexOf(".") + 1,
                ) ==
                "fromJson",
          )) {
        var instanceMirror = classMirror.newInstance("fromJson", [
          json.decode(data)["r"],
        ]);

        return instanceMirror as T?;
      }
    } on NoSuchCapabilityError catch (e) {
      Log.d(
        "[JsonSerializer : decodeFunctionReturnValue] Class mirror not found for type: $e",
      );
    } catch (e) {
      Log.e(
        "[JsonSerializer : decodeFunctionReturnValue] Error when find class mirror",
      );
      Log.d(
        "[JsonSerializer : decodeFunctionReturnValue] Error when find class mirror: $e",
      );
    }

    // Anything else is answered as the plain dart type the wire carried.
    try {
      return json.decode(data)["r"] as T?;
    } catch (e) {
      Log.e(
        "[JsonSerializer : decodeFunctionReturnValue] Error when parse json",
      );
      Log.d(
        "[JsonSerializer : decodeFunctionReturnValue] Error when parse json: $e",
      );
    }

    return null;
  }

  @override
  DecodedRequest? decodeRequest(String data) {
    try {
      final decodedData = json.decode(data) as Map<String, dynamic>;
      final message = Message();

      for (final param in (decodedData["p"] as List<dynamic>? ?? [])) {
        final entry = param as Map<String, dynamic>;
        message.set(entry["n"] as String, entry["v"]);
      }

      return DecodedRequest(decodedData["f"] as String? ?? "", message);
    } catch (e) {
      Log.e("[JsonSerializer : decodeRequest] Error when decode request");
      Log.d("[JsonSerializer : decodeRequest] Error when decode request: $e");
    }

    return null;
  }

  @override
  String encodeFunctionReturnValue(data) {
    try {
      return jsonEncode({"r": data});
    } catch (e) {
      Log.e(
        "[JsonSerializer : encodeFunctionReturnValue] Error when encode data",
      );
      Log.d(
        "[JsonSerializer : encodeFunctionReturnValue] Error when encode data: $e",
      );
    }

    return "";
  }

  @override
  String encodeRequest(String functionName, [List<Param>? params]) {
    try {
      return json.encode({"f": functionName, "p": params ?? []});
    } catch (e) {
      Log.e("[JsonSerializer : encodeRequest] Error when encode data");
      Log.d("[JsonSerializer : encodeRequest] Error when encode data: $e");
    }

    return "";
  }
}
