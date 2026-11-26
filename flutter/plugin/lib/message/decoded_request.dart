import 'package:xplpc/message/message.dart';

class DecodedRequest {
  final String functionName;
  final Message message;

  DecodedRequest(this.functionName, this.message);
}
