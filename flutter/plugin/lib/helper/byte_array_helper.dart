import 'dart:ffi' as ffi;
import 'dart:typed_data';

import 'package:xplpc/type/dataview.dart';

class ByteArrayHelper {
  static Uint8List createFromDataView(DataView dataView) {
    ffi.Pointer<ffi.Uint8> ptr = ffi.Pointer<ffi.Uint8>.fromAddress(
      dataView.ptr,
    );

    return ptr.asTypedList(dataView.size);
  }
}
