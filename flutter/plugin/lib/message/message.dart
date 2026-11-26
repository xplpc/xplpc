import 'package:xplpc/reflectable/json_constructor.dart';
import 'package:xplpc/util/log.dart';

class Message {
  final _data = <String, dynamic>{};

  T? get<T>(String name) {
    final value = _data[name];

    // A parameter may arrive with its name and no value, which is nothing to read rather than a value of another type.
    if (value == null) {
      return null;
    }

    if (value is T) {
      return value;
    }

    // A reflected type arrives as the document the wire carried, so it is built through its fromJson constructor the way the serializer builds an answer.
    final classMirror = JsonConstructor.find<T>();

    if (classMirror != null) {
      try {
        return classMirror.newInstance("fromJson", [value]) as T;
      } catch (e) {
        Log.e('[Message : get] Value of "$name" has another type');
        Log.d('[Message : get] Value of "$name" has another type: $e');

        return null;
      }
    }

    // Json has one kind of number, so an integer is read as a double exactly when the double holds it.
    if (value is int && 0.0 is T) {
      final converted = value.toDouble();

      if (converted.isFinite && BigInt.from(converted) == BigInt.from(value)) {
        return converted as T;
      }
    }

    Log.e('[Message : get] Value of "$name" has another type');

    return null;
  }

  void set(String name, dynamic value) {
    _data[name] = value;
  }
}
