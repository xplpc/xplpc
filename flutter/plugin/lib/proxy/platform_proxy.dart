import 'dart:ffi' as ffi;

import "package:ffi/ffi.dart";
import 'package:xplpc/client/proxy_client.dart';
import 'package:xplpc/core/xplpc.dart';
import 'package:xplpc/data/callback_list.dart';
import 'package:xplpc/type/typedefs.dart';

class PlatformProxy {
  static late InitializeFunc initializeFunc;
  static late IsInitializedFunc isInitializedFunc;
  static late CallProxyFunc nativeCallProxyFunc;
  static late CallProxyCallbackFunc nativeCallProxyCallbackFunc;

  static final callProxyCallbackFunc =
      ffi.Pointer.fromFunction<CallProxyCallback>(callProxyCallback);

  static final onNativeProxyCallFunc =
      ffi.Pointer.fromFunction<CallProxyCallback>(onNativeProxyCall);

  static void initialize() {
    // function: xplpc_core_initialize
    initializeFunc = XPLPC.instance.library
        .lookupFunction<NativeInitializeFunc, InitializeFunc>(
      'xplpc_core_initialize',
    );

    // function: xplpc_core_is_initialized
    isInitializedFunc = XPLPC.instance.library
        .lookupFunction<NativeIsInitializedFunc, IsInitializedFunc>(
      'xplpc_core_is_initialized',
    );

    // function: xplpc_native_call_proxy
    nativeCallProxyFunc = XPLPC.instance.library
        .lookupFunction<NativeCallProxyFunc, CallProxyFunc>(
      'xplpc_native_call_proxy',
    );

    // function: xplpc_native_call_proxy
    nativeCallProxyCallbackFunc = XPLPC.instance.library
        .lookupFunction<NativeCallProxyCallbackFunc, CallProxyCallbackFunc>(
      'xplpc_native_call_proxy_callback',
    );

    // callbacks
    initializeFunc(callProxyCallbackFunc, onNativeProxyCallFunc);
  }

  static void callProxyCallback(
    ffi.Pointer<Utf8> key,
    int keySize,
    ffi.Pointer<Utf8> data,
    int dataSize,
  ) {
    final keyStr = key.toDartString(length: keySize);
    final dataStr = data.toDartString(length: dataSize);

    CallbackList.instance.execute(keyStr, dataStr);
  }

  static void onNativeProxyCall(
    ffi.Pointer<Utf8> key,
    int keySize,
    ffi.Pointer<Utf8> data,
    int dataSize,
  ) {
    final dataStr = data.toDartString(length: dataSize);

    ProxyClient.call(dataStr, (String response) {
      nativeCallProxyCallbackFunc(
        key,
        key.length,
        response.toNativeUtf8(),
        response.length,
      );
    });
  }
}
