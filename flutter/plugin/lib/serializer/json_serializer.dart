import 'dart:convert';

import 'package:xplpc/message/decoded_request.dart';
import 'package:xplpc/message/message.dart';
import 'package:xplpc/message/param.dart';
import 'package:xplpc/reflectable/json_constructor.dart';
import 'package:xplpc/serializer/serializer.dart';
import 'package:xplpc/util/log.dart';

class JsonSerializer implements Serializer {
  @override
  T? decodeFunctionReturnValue<T>(String data) {
    // An empty response is the empty value every failing path answers with, not a document that failed to parse.
    if (data.isEmpty) {
      return null;
    }

    final dynamic document;

    try {
      document = json.decode(data);
    } catch (e) {
      Log.e(
        "[JsonSerializer : decodeFunctionReturnValue] Error when parse json",
      );
      Log.d(
        "[JsonSerializer : decodeFunctionReturnValue] Error when parse json: $e",
      );

      return null;
    }

    // A document that carries no value is the empty value, which is not a failure to report, and a mapping with nothing to answer travels as null.
    final value = document is Map<String, dynamic> ? document["r"] : null;

    if (value == null) {
      return null;
    }

    // The decoder reads a number past what a double holds as infinity wherever it sits, which no other bridge delivers.
    if (!_holdsOnlyFiniteNumbers(value)) {
      Log.e(
        "[JsonSerializer : decodeFunctionReturnValue] Value has another type",
      );
      return null;
    }

    // A reflected type is filled through the fromJson constructor it declares, and anything else is the plain dart type the wire carried.
    try {
      final classMirror = JsonConstructor.find<T>();

      if (classMirror != null) {
        return classMirror.newInstance("fromJson", [value]) as T?;
      }

      // The decoder keeps a json integer as an int, which is not a double, so an integer is read as a double when the double holds it exactly and is refused by the cast below otherwise.
      if (value is int && value is! T && 0.0 is T) {
        final widened = value.toDouble();

        if (BigInt.from(widened) == BigInt.from(value)) {
          return widened as T;
        }
      }

      return value as T?;
    } catch (e) {
      Log.e(
        "[JsonSerializer : decodeFunctionReturnValue] Value has another type",
      );
      Log.d(
        "[JsonSerializer : decodeFunctionReturnValue] Value has another type: $e",
      );
    }

    return null;
  }

  @override
  DecodedRequest? decodeRequest(String data) {
    try {
      final decodedData = json.decode(data);

      if (decodedData is! Map<String, dynamic>) {
        Log.e("[JsonSerializer : decodeRequest] Error when decode request");
        return null;
      }

      // A request that names no function as text routes nowhere, which is what the reference refuses before it ever reaches a decoder.
      final functionName = decodedData["f"];

      if (functionName is! String || functionName.isEmpty) {
        Log.e("[JsonSerializer : decodeRequest] The request names no function");
        return null;
      }

      final params = decodedData["p"] ?? <dynamic>[];

      if (params is! List<dynamic>) {
        Log.e(
          '[JsonSerializer : decodeRequest] Parameters of function "$functionName" are not a list',
        );
        return null;
      }

      final message = Message();
      final names = <String>{};

      for (final param in params) {
        final entry = param is Map<String, dynamic>
            ? param
            : const <String, dynamic>{};
        final name = entry["n"];

        if (name is! String) {
          Log.e(
            '[JsonSerializer : decodeRequest] Function "$functionName" carries a parameter whose name is not text',
          );
          return null;
        }

        // A name read twice means the first value to one bridge and the last to another, so the request is refused rather than read either way.
        if (!names.add(name)) {
          Log.e(
            '[JsonSerializer : decodeRequest] Parameter "$name" of function "$functionName" appears more than once',
          );
          return null;
        }

        message.set(name, entry["v"]);
      }

      return DecodedRequest(functionName, message);
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

  static bool _holdsOnlyFiniteNumbers(dynamic value) {
    if (value is double) {
      return value.isFinite;
    }

    if (value is List) {
      return value.every(_holdsOnlyFiniteNumbers);
    }

    if (value is Map) {
      return value.values.every(_holdsOnlyFiniteNumbers);
    }

    return true;
  }
}
