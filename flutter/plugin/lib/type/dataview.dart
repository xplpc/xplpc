import 'dart:ffi' as ffi;
import 'dart:typed_data';

import "package:ffi/ffi.dart";
import 'package:xplpc/reflectable/reflector.dart';
import 'package:xplpc/util/log.dart';

@reflector
class DataView {
  late int ptr;
  late int size;

  bool _owned = false;

  DataView(this.ptr, this.size);

  static DataView createFromByteBuffer(ffi.Pointer<ffi.Uint8> data, int size) {
    return DataView(data.address, size);
  }

  static DataView allocateFromByteArray(Uint8List data) {
    // The bytes are copied into native memory owned by the returned view, which is released with dispose.

    final blob = calloc<ffi.Uint8>(data.length);
    blob.asTypedList(data.length).setAll(0, data);

    return DataView(blob.address, data.length).._owned = true;
  }

  void dispose() {
    if (!_owned) {
      return;
    }

    calloc.free(ffi.Pointer<ffi.Uint8>.fromAddress(ptr));

    _owned = false;
    ptr = 0;
    size = 0;
  }

  Map<String, dynamic> toJson() {
    return {'ptr': ptr, 'size': size};
  }

  DataView.fromJson(Map<String, dynamic> json) {
    final rawPtr = json['ptr'];
    final rawSize = json['size'];

    if (rawPtr is! int || rawSize is! int) {
      Log.e("[DataView : fromJson] The value is not a data view");

      ptr = 0;
      size = 0;

      return;
    }

    ptr = rawPtr;
    size = rawSize;
  }
}
