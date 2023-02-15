import 'dart:ffi' as ffi;
import 'dart:io';

class XPLPC {
  static initialize() {
    // TODO: XPLPC - IMPLEMENT
  }

  static ffi.DynamicLibrary loadLibrary() {
    return Platform.isAndroid
        ? ffi.DynamicLibrary.open("libxplpc.so")
        : ffi.DynamicLibrary.process();
  }
}
