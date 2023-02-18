import 'package:xplpc/message/message.dart';
import 'package:xplpc/message/response.dart';

typedef Target = Function(Message m, Response r);

class MappingItem {
  Target target;

  MappingItem(this.target);
}
