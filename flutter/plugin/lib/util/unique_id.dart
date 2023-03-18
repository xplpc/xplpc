import 'package:synchronized/synchronized.dart';

class UniqueID {
  // singleton
  static UniqueID? _instance;
  UniqueID._();
  static UniqueID get instance => _instance ??= UniqueID._();

  // properties
  final lock = Lock();
  BigInt counter = BigInt.from(0);

  // methods
  static Future<String> generate() async {
    return await instance.lock.synchronized(() async {
      instance.counter = (instance.counter + BigInt.from(1));
      return "FL-${instance.counter}";
    });
  }
}
