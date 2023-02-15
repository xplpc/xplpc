import 'dart:async';
import 'dart:ffi' as ffi;

import "package:ffi/ffi.dart";
import 'package:flutter/material.dart';
import 'package:xplpc_plugin/core/xplpc.dart';

void main() {
  runApp(const MyApp());
}

class MyApp extends StatefulWidget {
  const MyApp({super.key});

  @override
  State<MyApp> createState() => _MyAppState();
}

// callback
typedef CallProxyCallback = ffi.Void Function(
  ffi.Pointer<Utf8> key,
  ffi.Int32 keySize,
  ffi.Pointer<Utf8> data,
  ffi.Int32 dataSize,
);

// ffi signature of the C functions
typedef NativeInitializeFunc = ffi.Void Function();
typedef NativeIsInitializedFunc = ffi.Bool Function();
typedef NativeCallProxyFunc = ffi.Void Function(
  ffi.Pointer<Utf8>,
  ffi.Size,
  ffi.Pointer<Utf8>,
  ffi.Size,
);

// dart type definition for calling the C function
typedef InitializeFunc = void Function();
typedef IsInitializedFunc = bool Function();
typedef CallProxyFunc = void Function(
  ffi.Pointer<Utf8>,
  int,
  ffi.Pointer<Utf8>,
  int,
);

void callProxyCallbackNative(
  ffi.Pointer<Utf8> key,
  int keySize,
  ffi.Pointer<Utf8> data,
  int dataSize,
) {
  final keyStr = key.toDartString(length: keySize);
  final dataStr = data.toDartString(length: dataSize);
  print("[callProxyCallbackNative] Key: $keyStr, Data: $dataStr");
}

void onNativeProxyCallNative(
  ffi.Pointer<Utf8> key,
  int keySize,
  ffi.Pointer<Utf8> data,
  int dataSize,
) {
  final keyStr = key.toDartString(length: keySize);
  final dataStr = data.toDartString(length: dataSize);
  print("[onNativeProxyCallNative] Key: $keyStr, Data: $dataStr");
}

class _MyAppState extends State<MyApp> {
  late int sumResult = 0;
  late Future<int> sumAsyncResult = Future(() => 0);

  @override
  void initState() {
    super.initState();
    //sumResult = xplpc.sum(1, 2);
    //sumAsyncResult = xplpc.sumAsync(3, 4);

    // load library
    XPLPC.initialize();
    final library = XPLPC.loadLibrary();

    // load
    final callProxyCallback = ffi.Pointer.fromFunction<CallProxyCallback>(
      callProxyCallbackNative,
    );

    final onNativeProxyCall = ffi.Pointer.fromFunction<CallProxyCallback>(
      onNativeProxyCallNative,
    );

    // look up the C function 'xplpc_core_initialize'

    final initializeFunc =
        library.lookupFunction<
                ffi.Void Function(
                    ffi.Pointer<ffi.NativeFunction<CallProxyCallback>>,
                    ffi.Pointer<ffi.NativeFunction<CallProxyCallback>>),
                void Function(
                    ffi.Pointer<ffi.NativeFunction<CallProxyCallback>>,
                    ffi.Pointer<ffi.NativeFunction<CallProxyCallback>>)>(
            'xplpc_core_initialize');

    // call the function
    print("Initializing...");
    initializeFunc(callProxyCallback, onNativeProxyCall);
    print("Initialized");

    // look up the C function 'xplpc_core_is_initialized'
    final IsInitializedFunc isInitializedFunc = library
        .lookup<ffi.NativeFunction<NativeIsInitializedFunc>>(
          'xplpc_core_is_initialized',
        )
        .asFunction();

    // call the function
    print("Is Initialized...");
    bool isInitialized = isInitializedFunc();
    print("Is Initialized: $isInitialized");

    // look up the C function 'xplpc_native_call_proxy'
    final CallProxyFunc callProxyFunc = library
        .lookup<ffi.NativeFunction<NativeCallProxyFunc>>(
          'xplpc_native_call_proxy',
        )
        .asFunction();

    // call the function
    print("Calling proxy...");

    String key = "1";
    String data =
        '{"f":"sample.login","p":[{"n":"username","v":"paulo"},{"n":"password","v":"123456"},{"n":"remember","v":true}]}';

    callProxyFunc(
      key.toNativeUtf8(),
      key.length,
      data.toNativeUtf8(),
      data.length,
    );

    print("Called");
  }

  @override
  Widget build(BuildContext context) {
    const textStyle = TextStyle(fontSize: 25);
    const spacerSmall = SizedBox(height: 10);
    return MaterialApp(
      home: Scaffold(
        appBar: AppBar(
          title: const Text('Native Packages'),
        ),
        body: SingleChildScrollView(
          child: Container(
            padding: const EdgeInsets.all(10),
            child: Column(
              children: [
                const Text(
                  'This calls a native function through FFI that is shipped as source in the package. '
                  'The native code is built as part of the Flutter Runner build.',
                  style: textStyle,
                  textAlign: TextAlign.center,
                ),
                spacerSmall,
                Text(
                  'sum(1, 2) = $sumResult',
                  style: textStyle,
                  textAlign: TextAlign.center,
                ),
                spacerSmall,
                FutureBuilder<int>(
                  future: sumAsyncResult,
                  builder: (BuildContext context, AsyncSnapshot<int> value) {
                    final displayValue =
                        (value.hasData) ? value.data : 'loading';
                    return Text(
                      'await sumAsync(3, 4) = $displayValue',
                      style: textStyle,
                      textAlign: TextAlign.center,
                    );
                  },
                ),
              ],
            ),
          ),
        ),
      ),
    );
  }
}
