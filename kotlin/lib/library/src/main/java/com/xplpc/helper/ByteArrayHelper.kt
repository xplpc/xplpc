package com.xplpc.helper

import com.xplpc.type.DataView

object ByteArrayHelper {
    external fun getPtrAddress(data: ByteArray): Long

    @SuppressWarnings("WeakerAccess")
    external fun createFromPtr(ptr: Long, size: Int): ByteArray

    fun createFromDataView(dataView: DataView): ByteArray {
        return createFromPtr(dataView.ptr, dataView.size)
    }
}
