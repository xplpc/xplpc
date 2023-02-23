import 'dart:ffi' as ffi;
import 'dart:typed_data';

import "package:ffi/ffi.dart";
import 'package:xplpc/reflectable/reflector.dart';

@reflector
class DataView {
  late int ptr;
  late int size;

  DataView(this.ptr, this.size);

  static DataView createFromByteBuffer(ffi.Pointer<ffi.Uint8> data, int size) {
    return DataView(data.address, size);
  }

  static DataView createFromByteArray(Uint8List data) {
    final blob = calloc<ffi.Uint8>(data.length);
    final blobBytes = blob.asTypedList(data.length);
    blobBytes.setAll(0, data);
    return DataView(blob.address, data.length);
  }

  Map<String, dynamic> toJson() {
    return {
      'ptr': ptr,
      'size': size,
    };
  }

  DataView.fromJson(Map<String, dynamic> json) {
    ptr = json['ptr'];
    size = json['size'];
  }
}
