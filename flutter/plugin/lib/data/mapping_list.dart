import 'package:xplpc/core/xplpc.dart';
import 'package:xplpc/layer/layer.dart';
import 'package:xplpc/map/mapping_item.dart';
import 'package:xplpc/util/log.dart';

class MappingList {
  static MappingList? _instance;
  MappingList._();
  static MappingList get instance => _instance ??= MappingList._();

  final _list = <String, MappingItem>{};

  void add(String name, MappingItem item) {
    // A name is declared to a layer the library attaches when it comes up, so registering before that is a mistake in the program and says so by name.
    if (!XPLPC.instance.initialized) {
      Log.e("[MappingList : add] XPLPC was not initialized");
      throw StateError("XPLPC was not initialized");
    }

    // The router may hand a call over the moment it learns a name, so the mapping is recorded first and taken back when the router refuses it.

    final previous = _list[name];
    _list[name] = item;

    try {
      Layer.declare(name);
    } catch (_) {
      if (previous == null) {
        _list.remove(name);
      } else {
        _list[name] = previous;
      }

      rethrow;
    }
  }

  MappingItem? find(String name) {
    return _list[name];
  }

  bool has(String name) {
    return _list.containsKey(name);
  }

  void clear() {
    // The reverse of add, so the router stops handing names over before the mappings behind them are gone.
    for (final name in _list.keys) {
      Layer.retract(name);
    }

    _list.clear();
  }

  int count() {
    return _list.length;
  }
}
