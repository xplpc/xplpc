package com.xplpc.type

import com.xplpc.helper.ByteBufferHelper
import java.nio.ByteBuffer

@Suppress("unused")
class DataView(
    var ptr: Long,
    var size: Int
) {
    companion object {
        fun createFromByteBuffer(data: ByteBuffer): DataView {
            val ptr = ByteBufferHelper.getPtrAddress(data)
            return DataView(ptr, data.capacity())
        }
    }
}
