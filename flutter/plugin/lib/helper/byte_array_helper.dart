import 'dart:ffi' as ffi;
import 'dart:typed_data';

import 'package:xplpc/type/dataview.dart';

class ByteArrayHelper {
  static Uint8List createFromDataView(DataView dataView) {
    // A view that carries no address describes nothing to read, and reading it anyway takes the isolate down.

    if (dataView.ptr == 0 || dataView.size <= 0) {
      return Uint8List(0);
    }

    return ffi.Pointer<ffi.Uint8>.fromAddress(
      dataView.ptr,
    ).asTypedList(dataView.size);
  }
}
