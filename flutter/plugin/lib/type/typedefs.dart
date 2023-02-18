import 'dart:ffi' as ffi;

import "package:ffi/ffi.dart";

// ffi callback signature of the C functions
typedef CallProxyCallback = ffi.Void Function(
  ffi.Pointer<Utf8>,
  ffi.Int32,
  ffi.Pointer<Utf8>,
  ffi.Int32,
);

// ffi signature of the C functions
typedef NativeInitializeFunc = ffi.Void Function(
  ffi.Pointer<ffi.NativeFunction<CallProxyCallback>>,
  ffi.Pointer<ffi.NativeFunction<CallProxyCallback>>,
);

typedef NativeIsInitializedFunc = ffi.Bool Function();

typedef NativeCallProxyFunc = ffi.Void Function(
  ffi.Pointer<Utf8>,
  ffi.Size,
  ffi.Pointer<Utf8>,
  ffi.Size,
);

typedef NativeCallProxyCallbackFunc = ffi.Void Function(
  ffi.Pointer<Utf8>,
  ffi.Size,
  ffi.Pointer<Utf8>,
  ffi.Size,
);

// dart type definition for calling the C functions
typedef InitializeFunc = void Function(
  ffi.Pointer<ffi.NativeFunction<CallProxyCallback>>,
  ffi.Pointer<ffi.NativeFunction<CallProxyCallback>>,
);

typedef IsInitializedFunc = bool Function();

typedef CallProxyFunc = void Function(
  ffi.Pointer<Utf8>,
  int,
  ffi.Pointer<Utf8>,
  int,
);

typedef CallProxyCallbackFunc = void Function(
  ffi.Pointer<Utf8>,
  int,
  ffi.Pointer<Utf8>,
  int,
);

// dart client callbacks
typedef LocalClientCallback<T> = void Function(T);
typedef ProxyClientCallback = void Function(String);
typedef RemoteClientCallback<T> = void Function(T?);
