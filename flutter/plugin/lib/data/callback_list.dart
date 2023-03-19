import 'package:synchronized/synchronized.dart';

class CallbackList {
  // singleton
  static CallbackList? _instance;
  CallbackList._();
  static CallbackList get instance => _instance ??= CallbackList._();

  // properties
  var list = <String, void Function(String)>{};
  var lock = Lock(reentrant: true);

  // methods
  void add(String key, void Function(String) callback) async {
    await lock.synchronized(() async {
      list[key] = callback;
    });
  }

  void execute(String key, String data) async {
    void Function(String)? callback;

    await lock.synchronized(() async {
      if (list.containsKey(key)) {
        callback = list[key];
        list.remove(key);
      }
    });

    callback?.call(data);
  }

  Future<int> count() async {
    return await lock.synchronized(() async {
      return list.length;
    });
  }
}
