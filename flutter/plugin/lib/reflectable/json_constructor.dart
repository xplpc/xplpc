import 'package:reflectable/reflectable.dart';
import 'package:xplpc/reflectable/reflector.dart';

// Builds a reflected type from the value the wire carried, which is how a type crosses the bridge without a generator of its own.
class JsonConstructor {
  // Answers null for a type that is not reflected or declares no fromJson constructor, since the wire value is then read as it is.
  static ClassMirror? find<T>() {
    final bool reflected;

    try {
      reflected = reflector.canReflectType(T);
    } on StateError {
      // Reflection is generated per program, and one that never initialized it reads no reflected type.
      return null;
    }

    if (!reflected) {
      return null;
    }

    final mirror = reflector.reflectType(T);

    if (mirror is! ClassMirror) {
      return null;
    }

    // A named constructor is reported as the class name and the constructor name joined by a dot.
    final declaresFromJson = mirror.declarations.values
        .whereType<MethodMirror>()
        .any((m) => m.simpleName.split(".").last == "fromJson");

    return declaresFromJson ? mirror : null;
  }
}
