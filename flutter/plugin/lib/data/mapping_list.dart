import 'package:xplpc/map/mapping_item.dart';
import 'package:xplpc/proxy/platform_proxy.dart';

class MappingList {
  static MappingList? _instance;
  MappingList._();
  static MappingList get instance => _instance ??= MappingList._();

  final _list = <String, MappingItem>{};

  void add(String name, MappingItem item) {
    // The native side routes from the names it was told, so a failure to declare one must leave nothing behind to be unreachable later.
    PlatformProxy.addMapping(name);
    _list[name] = item;
  }

  MappingItem? find(String name) {
    return _list[name];
  }

  bool has(String name) {
    return _list.containsKey(name);
  }

  void clear() {
    PlatformProxy.clearMappings();
    _list.clear();
  }

  int count() {
    return _list.length;
  }
}
