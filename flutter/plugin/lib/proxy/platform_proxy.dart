import 'dart:ffi' as ffi;

import "package:ffi/ffi.dart";
import 'package:xplpc/core/xplpc.dart';
import 'package:xplpc/data/callback_list.dart';
import 'package:xplpc/data/mapping_list.dart';
import 'package:xplpc/message/message.dart';
import 'package:xplpc/type/typedefs.dart';
import 'package:xplpc/util/log.dart';

class PlatformProxy {
  static late InitializeFunc initializeFunc;
  static late IsInitializedFunc isInitializedFunc;
  static late CallProxyFunc nativeCallProxyFunc;
  static late CallProxyCallbackFunc nativeCallProxyCallbackFunc;

  static final onInitializePlatformFunc =
      ffi.Pointer.fromFunction<OnInitializePlatform>(onInitializePlatform);

  static final onFinalizePlatformFunc =
      ffi.Pointer.fromFunction<OnFinalizePlatform>(onFinalizePlatform);

  static final onHasMappingFunc =
      ffi.Pointer.fromFunction<OnHasMapping>(onHasMapping, false);

  static final onNativeProxyCallFunc =
      ffi.Pointer.fromFunction<CallProxyCallback>(onNativeProxyCall);

  static final onNativeProxyCallbackFunc =
      ffi.Pointer.fromFunction<CallProxyCallback>(onNativeProxyCallback);

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
    initializeFunc(
      onInitializePlatformFunc,
      onFinalizePlatformFunc,
      onHasMappingFunc,
      onNativeProxyCallFunc,
      onNativeProxyCallbackFunc,
    );
  }

  static void onNativeProxyCall(
    ffi.Pointer<Utf8> key,
    int keySize,
    ffi.Pointer<Utf8> data,
    int dataSize,
  ) {
    final keyStr = key.toDartString(length: keySize);
    final dataStr = data.toDartString(length: dataSize);

    // function name
    final functionName = XPLPC.instance.config.serializer.decodeFunctionName(
      dataStr,
    );

    if (functionName.isEmpty) {
      Log.e("[PlatformProxy : call] Function name is empty");
      _callNativeProxyWith(keyStr, "");
      return;
    }

    // mapping item
    final mappingItem = MappingList.instance.find(functionName);

    if (mappingItem == null) {
      Log.e(
        "[PlatformProxy : call] Mapping not found for function: $functionName",
      );
      _callNativeProxyWith(keyStr, "");
      return;
    }

    // execute
    Message? message;

    try {
      message = XPLPC.instance.config.serializer.decodeMessage(dataStr);
    } catch (e) {
      Log.e("[PlatformProxy : call] Error when decode message: $e");
    }

    if (message == null) {
      Log.e(
        "[PlatformProxy : call] Error when decode message for function: $functionName",
      );
      _callNativeProxyWith(keyStr, "");
      return;
    }

    try {
      mappingItem.target(message, (dynamic r) {
        final decodedData =
            XPLPC.instance.config.serializer.encodeFunctionReturnValue(r);

        _callNativeProxyWith(keyStr, decodedData);
      });
    } catch (e) {
      Log.e("[PlatformProxy : call] Error: $e");
      _callNativeProxyWith(keyStr, "");
    }
  }

  static void onNativeProxyCallback(
    ffi.Pointer<Utf8> key,
    int keySize,
    ffi.Pointer<Utf8> data,
    int dataSize,
  ) {
    final keyStr = key.toDartString(length: keySize);
    final dataStr = data.toDartString(length: dataSize);

    CallbackList.instance.execute(keyStr, dataStr);
  }

  static void onInitializePlatform() {
    // ignore
  }

  static void onFinalizePlatform() {
    MappingList.instance.clear();
  }

  static bool onHasMapping(ffi.Pointer<Utf8> name, int nameSize) {
    final nameStr = name.toDartString(length: nameSize);
    return MappingList.instance.has(nameStr);
  }

  static void _callNativeProxyWith(String key, String data) {
    final nativeKey = key.toNativeUtf8();
    final nativeData = data.toNativeUtf8();

    nativeCallProxyCallbackFunc(
      nativeKey,
      nativeKey.length,
      nativeData,
      nativeData.length,
    );
  }
}
