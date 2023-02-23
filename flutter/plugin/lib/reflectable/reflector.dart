import 'package:reflectable/reflectable.dart';

class Reflector extends Reflectable {
  const Reflector()
      : super(
          newInstanceCapability,
          invokingCapability,
          declarationsCapability,
          reflectedTypeCapability,
        );
}

const reflector = Reflector();
