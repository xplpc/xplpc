class CallbackList {
  static CallbackList? _instance;
  CallbackList._();
  static CallbackList get instance => _instance ??= CallbackList._();

  final _list = <String, void Function(String)>{};

  void add(String key, void Function(String) callback) {
    _list[key] = callback;
  }

  void execute(String key, String data) {
    // The native side may answer before this call returns, so registration and lookup must stay synchronous.

    _list.remove(key)?.call(data);
  }

  void remove(String key) {
    _list.remove(key);
  }

  void clear() {
    _list.clear();
  }

  int count() {
    return _list.length;
  }
}
