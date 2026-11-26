import 'package:xplpc/message/message.dart';
import 'package:xplpc/message/response.dart';

typedef Target = void Function(Message m, Response r);

class MappingItem {
  final Target target;

  MappingItem(this.target);
}
