import 'package:xplpc/core/xplpc.dart';
import 'package:xplpc/message/param.dart';

class Request {
  final String functionName;
  final List<Param>? params;

  Request(this.functionName, [this.params]);

  String data() {
    return XPLPC.instance.config.serializer.encodeRequest(functionName, params);
  }
}
