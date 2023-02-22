import 'package:reflectable/reflectable.dart';

class Reflector extends Reflectable {
  const Reflector()
      : super(
          declarationsCapability,
          invokingCapability,
          metadataCapability,
          newInstanceCapability,
          typeRelationsCapability,
          typeCapability,
          reflectedTypeCapability,
        );
}

const reflector = Reflector();
