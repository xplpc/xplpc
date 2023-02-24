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

    initializeLibrary();
    initializePlatformProxy();
  }

  void initializeLibrary() {
    var openDirect = false;
    late String openPath;

    if (Platform.isAndroid) {
      openPath = "libxplpc.so";
    } else if (Platform.isWindows) {
      openPath = "xplpc.dll";
    } else if (Platform.isLinux) {
      openPath = "libxplpc.so";
    } else {
      openDirect = true;
    }

    if (Platform.environment.containsKey('FLUTTER_TEST')) {
      openDirect = false;
      openPath =
          "../../build/flutter-macos-xcframework/xplpc.xcframework/macos-arm64_x86_64/xplpc.framework/Versions/A/xplpc";
    }

    if (openDirect) {
      library = ffi.DynamicLibrary.process();
    } else {
      library = ffi.DynamicLibrary.open(openPath);
    }
  }

  void initializePlatformProxy() {
    PlatformProxy.initialize();
  }
}
