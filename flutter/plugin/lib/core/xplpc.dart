import 'dart:ffi' as ffi;
import 'dart:io';

import 'package:xplpc/core/config.dart';
import 'package:xplpc/proxy/platform_proxy.dart';

class XPLPC {
  // singleton
  static XPLPC? _instance;
  XPLPC._();
  static XPLPC get instance => _instance ??= XPLPC._();

  // native library
  late ffi.DynamicLibrary library;

  // properties
  bool initialized = false;
  late Config config;

  // methods
  void initialize(Config config) {
    if (initialized) {
      return;
    }

    initialized = true;

    this.config = config;

    loadLibrary();
  }

  void loadLibrary() {
    library = Platform.isAndroid
        ? ffi.DynamicLibrary.open("libxplpc.so")
        : ffi.DynamicLibrary.process();

    PlatformProxy.initialize();
  }
}
