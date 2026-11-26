import 'dart:ffi' as ffi;

import "package:ffi/ffi.dart";

final class NativeHost extends ffi.Struct {
  external ffi.Pointer<ffi.Void> context;
  external ffi.Pointer<ffi.NativeFunction<InvokeCallback>> invoke;
  external ffi.Pointer<ffi.NativeFunction<PostCallback>> post;

  @ffi.Bool()
  external bool enterableAnywhere;

  external ffi.Pointer<Utf8> name;

  @ffi.Size()
  external int nameSize;
}

final class NativeCallResult extends ffi.Struct {
  @ffi.Uint64()
  external int call;

  external ffi.Pointer<Utf8> data;

  @ffi.Size()
  external int dataSize;
}

typedef InvokeCallback = ffi.Void Function(
  ffi.Pointer<ffi.Void>,
  ffi.Uint64,
  ffi.Pointer<Utf8>,
  ffi.Size,
);

typedef PostCallback = ffi.Void Function(
  ffi.Pointer<ffi.Void>,
  ffi.Int,
  ffi.Uint64,
  ffi.Pointer<Utf8>,
  ffi.Size,
);

typedef NativeAttachFunc = ffi.Uint64 Function(ffi.Pointer<NativeHost>);

typedef NativeNameFunc = ffi.Void Function(
  ffi.Uint64,
  ffi.Pointer<Utf8>,
  ffi.Size,
);

typedef NativeCallFunc = ffi.Bool Function(
  ffi.Uint64,
  ffi.Uint64,
  ffi.Pointer<Utf8>,
  ffi.Size,
  ffi.Pointer<NativeCallResult>,
);

typedef NativeCancelFunc = ffi.Bool Function(ffi.Uint64, ffi.Uint64);

typedef NativeAnswerFunc = ffi.Void Function(
  ffi.Uint64,
  ffi.Pointer<Utf8>,
  ffi.Size,
);

typedef NativeBufferRegisterFunc = ffi.Uint64 Function(
  ffi.Uint64,
  ffi.Pointer<ffi.Void>,
  ffi.Size,
);

typedef NativeBufferReadFunc = ffi.Bool Function(
  ffi.Uint64,
  ffi.Pointer<ffi.Pointer<ffi.Void>>,
  ffi.Pointer<ffi.Size>,
);

typedef NativeBufferReleaseFunc = ffi.Bool Function(ffi.Uint64, ffi.Uint64);

typedef NativePendingFunc = ffi.Size Function();

typedef NativeFreeFunc = ffi.Void Function(ffi.Pointer<ffi.Void>);

typedef AttachFunc = int Function(ffi.Pointer<NativeHost>);

typedef NameFunc = void Function(int, ffi.Pointer<Utf8>, int);

typedef CallFunc = bool Function(
  int,
  int,
  ffi.Pointer<Utf8>,
  int,
  ffi.Pointer<NativeCallResult>,
);

typedef CancelFunc = bool Function(int, int);

typedef AnswerFunc = void Function(int, ffi.Pointer<Utf8>, int);

typedef BufferRegisterFunc = int Function(int, ffi.Pointer<ffi.Void>, int);

typedef BufferReadFunc = bool Function(
  int,
  ffi.Pointer<ffi.Pointer<ffi.Void>>,
  ffi.Pointer<ffi.Size>,
);

typedef BufferReleaseFunc = bool Function(int, int);

typedef PendingFunc = int Function();

typedef FreeFunc = void Function(ffi.Pointer<ffi.Void>);

typedef ClientCallback<T> = void Function(T?)?;
typedef ClientCallbackFromString = void Function(String)?;
