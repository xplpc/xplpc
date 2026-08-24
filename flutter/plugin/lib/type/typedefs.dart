import 'dart:ffi' as ffi;
import "package:ffi/ffi.dart";

typedef OnInitializePlatform = ffi.Void Function();

typedef OnFinalizePlatform = ffi.Void Function();

typedef CallProxyCallback =
    ffi.Void Function(ffi.Pointer<Utf8>, ffi.Size, ffi.Pointer<Utf8>, ffi.Size);

typedef NativeInitializeFunc =
    ffi.Void Function(
      ffi.Bool,
      ffi.Pointer<ffi.NativeFunction<OnInitializePlatform>>,
      ffi.Pointer<ffi.NativeFunction<OnFinalizePlatform>>,
      ffi.Pointer<ffi.NativeFunction<CallProxyCallback>>,
      ffi.Pointer<ffi.NativeFunction<CallProxyCallback>>,
      ffi.Pointer<ffi.NativeFunction<CallProxyCallback>>,
      ffi.Pointer<ffi.NativeFunction<CallProxyCallback>>,
    );

typedef NativeCallProxyFunc =
    ffi.Void Function(ffi.Pointer<Utf8>, ffi.Size, ffi.Pointer<Utf8>, ffi.Size);

typedef NativeCallProxyCallbackFunc =
    ffi.Void Function(ffi.Pointer<Utf8>, ffi.Size, ffi.Pointer<Utf8>, ffi.Size);

typedef NativeMappingFunc = ffi.Void Function(ffi.Pointer<Utf8>, ffi.Size);

typedef NativeClearMappingsFunc = ffi.Void Function();

typedef NativeFreeFunc = ffi.Void Function(ffi.Pointer<ffi.Void>);

typedef InitializeFunc =
    void Function(
      bool,
      ffi.Pointer<ffi.NativeFunction<OnInitializePlatform>>,
      ffi.Pointer<ffi.NativeFunction<OnFinalizePlatform>>,
      ffi.Pointer<ffi.NativeFunction<CallProxyCallback>>,
      ffi.Pointer<ffi.NativeFunction<CallProxyCallback>>,
      ffi.Pointer<ffi.NativeFunction<CallProxyCallback>>,
      ffi.Pointer<ffi.NativeFunction<CallProxyCallback>>,
    );

typedef CallProxyFunc =
    void Function(ffi.Pointer<Utf8>, int, ffi.Pointer<Utf8>, int);

typedef CallProxyCallbackFunc =
    void Function(ffi.Pointer<Utf8>, int, ffi.Pointer<Utf8>, int);

typedef MappingFunc = void Function(ffi.Pointer<Utf8>, int);

typedef ClearMappingsFunc = void Function();

typedef FreeFunc = void Function(ffi.Pointer<ffi.Void>);

typedef ClientCallback<T> = void Function(T?)?;
typedef ClientCallbackFromString = void Function(String)?;
