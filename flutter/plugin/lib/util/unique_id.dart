class UniqueID {
  static int _counter = 0;

  static String generate() {
    // An isolate runs dart code on a single thread, so a plain counter is already race free.

    _counter += 1;
    return "FL-$_counter";
  }
}
