import 'package:xplpc/util/log.dart';

class Message {
  final _data = <String, dynamic>{};

  T? get<T>(String name) {
    if (!_data.containsKey(name)) {
      return null;
    }

    final value = _data[name];

    if (value is T) {
      return value;
    }

    Log.e('[Message : get] Value of "$name" has another type');

    return null;
  }

  void set(String name, dynamic value) {
    _data[name] = value;
  }
}
