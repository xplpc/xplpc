import 'package:xplpc/data/mapping_list.dart';
import 'package:xplpc/map/mapping_item.dart';
import 'package:xplpc/message/message.dart';
import 'package:xplpc/message/response.dart';

class Mapping {
  // singleton
  static Mapping? _instance;
  Mapping._();
  static Mapping get instance => _instance ??= Mapping._();

  // methods
  void initialize() {
    MappingList.instance.add(
      "platform.battery.level",
      MappingItem(batteryLevel),
    );
  }

  void batteryLevel(Message m, Response r) {
    var suffix = m.get("suffix");
    r("100$suffix");
  }
}
