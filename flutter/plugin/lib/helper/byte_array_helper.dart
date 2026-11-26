import 'dart:typed_data';

import 'package:xplpc/layer/layer.dart';
import 'package:xplpc/type/dataview.dart';

class ByteArrayHelper {
  // A view that names nothing, was released or was never registered answers no bytes, and the bytes it does answer are a copy taken while the owner still held them.
  static Uint8List createFromDataView(DataView dataView) {
    return Layer.readBuffer(dataView.buffer);
  }
}
