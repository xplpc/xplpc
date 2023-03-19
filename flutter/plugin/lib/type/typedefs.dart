import 'dart:ffi' as ffi;

import "package:ffi/ffi.dart";

// ffi callback signature of the C functions
typedef OnInitializePlatform = ffi.Void Function();

typedef OnFinalizePlatform = ffi.Void Function();

typedef OnHasMapping = ffi.Bool Function(
  ffi.Pointer<Utf8>,
  ffi.Int32,
);

typedef CallProxyCallback = ffi.Void Function(
  ffi.Pointer<Utf8>,
  ffi.Int32,
  ffi.Pointer<Utf8>,
  ffi.Int32,
);

// ffi signature of the C functions
typedef NativeInitializeFunc = ffi.Void Function(
  ffi.Bool,
  ffi.Pointer<ffi.NativeFunction<OnInitializePlatform>>,
  ffi.Pointer<ffi.NativeFunction<OnFinalizePlatform>>,
  ffi.Pointer<ffi.NativeFunction<OnHasMapping>>,
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

// dart type definitions
typedef InitializeFunc = void Function(
  bool,
  ffi.Pointer<ffi.NativeFunction<OnInitializePlatform>>,
  ffi.Pointer<ffi.NativeFunction<OnFinalizePlatform>>,
  ffi.Pointer<ffi.NativeFunction<OnHasMapping>>,
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

// dart client callback
typedef ClientCallback<T> = void Function(T)?;
typedef ClientCallbackFromString = void Function(String)?;
