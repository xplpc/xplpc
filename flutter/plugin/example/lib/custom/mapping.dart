import 'package:battery_plus/battery_plus.dart';
import 'package:xplpc/data/mapping_list.dart';
import 'package:xplpc/map/mapping_item.dart';
import 'package:xplpc/message/message.dart';
import 'package:xplpc/message/response.dart';

class Mapping {
  static Mapping? _instance;
  Mapping._();
  static Mapping get instance => _instance ??= Mapping._();

  void initialize() {
    MappingList.instance.add(
      "platform.battery.level",
      MappingItem(batteryLevel),
    );
  }

  void batteryLevel(Message m, Response r) {
    // The level arrives later, so a platform without a battery service has to answer too rather than leave the caller waiting.

    final suffix = m.get<String>("suffix") ?? "";

    Battery().batteryLevel.then(
      (level) => r("$level$suffix"),
      onError: (Object _) => r(null),
    );
  }
}
