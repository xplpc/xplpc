import 'package:reflectable/reflectable.dart';
import 'package:xplpc/reflectable/reflector.dart';

class CodableTypeList {
  // singleton
  static CodableTypeList? _instance;
  CodableTypeList._();
  static CodableTypeList get instance => _instance ??= CodableTypeList._();

  // properties
  var list = <TypeMirror>{};

  // methods
  void add<T>() {
    list.add(reflector.reflectType(T));
  }
}
