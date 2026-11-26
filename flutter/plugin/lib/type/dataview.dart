import 'dart:ffi' as ffi;
import 'dart:typed_data';

import "package:ffi/ffi.dart";
import 'package:xplpc/layer/layer.dart';
import 'package:xplpc/reflectable/reflector.dart';
import 'package:xplpc/util/log.dart';

// Names a buffer its owner registered, which is all that crosses the wire, so a reader can never be pointed at memory nobody registered.
@reflector
class DataView {
  late int buffer;

  ffi.Pointer<ffi.Uint8>? _allocation;

  bool _registered = false;

  DataView(this.buffer);

  // Borrows native memory the caller owns, which has to stay valid and in place until the view is disposed.
  static DataView createFromByteBuffer(ffi.Pointer<ffi.Uint8> data, int size) {
    return DataView(Layer.registerBuffer(data, size)).._registered = true;
  }

  // Copies the bytes into native memory the view owns and frees when it is disposed.
  static DataView allocateFromByteArray(Uint8List data) {
    final allocation = calloc<ffi.Uint8>(data.length);
    allocation.asTypedList(data.length).setAll(0, data);

    final int buffer;

    try {
      buffer = Layer.registerBuffer(allocation, data.length);
    } catch (_) {
      calloc.free(allocation);
      rethrow;
    }

    if (buffer == 0) {
      calloc.free(allocation);
      return DataView(0);
    }

    return DataView(buffer)
      .._allocation = allocation
      .._registered = true;
  }

  // Releases the registration before freeing what the view allocated, so no reader can reach bytes that are gone, and answers false for a view that did not register its buffer, even one naming a buffer this side registered, or that was already disposed.
  bool dispose() {
    if (!_registered) {
      return false;
    }

    _registered = false;

    final released = Layer.releaseBuffer(buffer);
    final allocation = _allocation;

    if (allocation != null) {
      calloc.free(allocation);
      _allocation = null;
    }

    return released;
  }

  Map<String, dynamic> toJson() {
    return {'buffer': buffer};
  }

  // This is how a mapping turns the value it received into a view, so what the wire carried decides the answer rather than the caller.
  DataView.fromJson(Object? json) {
    final value = json is Map<String, dynamic> ? json['buffer'] : null;

    if (value is! int || value < 0) {
      Log.e("[DataView : fromJson] Value is not a data view");
      Log.d(
        "[DataView : fromJson] Value is not a data view: ${json.runtimeType}",
      );

      buffer = 0;

      return;
    }

    buffer = value;
  }
}
