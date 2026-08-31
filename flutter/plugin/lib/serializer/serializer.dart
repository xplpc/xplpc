import 'package:xplpc/message/decoded_request.dart';
import 'package:xplpc/message/param.dart';

abstract class Serializer {
  DecodedRequest? decodeRequest(String data);
  String encodeFunctionReturnValue(dynamic data);
  T? decodeFunctionReturnValue<T>(String data);
  String encodeRequest(String functionName, [List<Param>? params]);
}
