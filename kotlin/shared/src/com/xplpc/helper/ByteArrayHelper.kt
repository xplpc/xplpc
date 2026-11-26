package com.xplpc.helper

import com.xplpc.layer.Layer
import com.xplpc.type.DataView

object ByteArrayHelper {
    // Copies the bytes a view names while its owner keeps them registered, and answers an empty array for a view that names nothing or whose buffer was released.
    fun createFromDataView(dataView: DataView): ByteArray {
        if (dataView.buffer == 0L) {
            return ByteArray(0)
        }

        return Layer.readBuffer(dataView.buffer) ?: ByteArray(0)
    }
}
