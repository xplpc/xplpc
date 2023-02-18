import 'package:xplpc/message/message.dart';
import 'package:xplpc/message/param.dart';

abstract class Serializer {
  String decodeFunctionName(String data);
  Message? decodeMessage(String data);
  String encodeFunctionReturnValue(dynamic data);
  T? decodeFunctionReturnValue<T>(String data);
  String encodeRequest(String functionName, [List<Param>? params]);
}
