class Message {
  var data = <String, dynamic>{};

  T? get<T>(String name) {
    if (data.containsKey(name)) {
      return data[name];
    }

    return null;
  }

  void set(String name, dynamic value) {
    data[name] = value;
  }
}
