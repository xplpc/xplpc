import 'package:xplpc/map/mapping_item.dart';

class MappingList {
  // singleton
  static MappingList? _instance;
  MappingList._();
  static MappingList get instance => _instance ??= MappingList._();

  // properties
  var list = <String, MappingItem>{};

  // methods
  void add(String name, MappingItem item) {
    list[name] = item;
  }

  MappingItem? find(String name) {
    if (list.containsKey(name)) {
      return list[name];
    }

    return null;
  }
}
