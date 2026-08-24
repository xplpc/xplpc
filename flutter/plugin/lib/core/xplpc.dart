import 'dart:ffi' as ffi;
import 'dart:io';

import 'package:xplpc/core/config.dart';
import 'package:xplpc/proxy/platform_proxy.dart';

class XPLPC {
  static XPLPC? _instance;
  XPLPC._();
  static XPLPC get instance => _instance ??= XPLPC._();

  late ffi.DynamicLibrary library;

  bool initialized = false;
  late Config config;

  void initialize(Config config) {
    if (initialized) {
      return;
    }

    this.config = config;

    library = _openLibrary();
    PlatformProxy.initialize();

    // This is only set once the library is loaded and the proxy is bound, since a failure above must not leave a half built singleton.
    initialized = true;
  }

  ffi.DynamicLibrary _openLibrary() {
    final path = Platform.environment["XPLPC_LIBRARY_PATH"];

    if (path != null && path.isNotEmpty) {
      return ffi.DynamicLibrary.open(path);
    }

    if (Platform.isAndroid || Platform.isLinux) {
      return ffi.DynamicLibrary.open("libxplpc.so");
    }

    if (Platform.isWindows) {
      return ffi.DynamicLibrary.open("xplpc.dll");
    }

    return ffi.DynamicLibrary.process();
  }
}
