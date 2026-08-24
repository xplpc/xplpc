package com.xplpc.type

import com.xplpc.helper.ByteBufferHelper
import com.xplpc.util.Log
import java.nio.ByteBuffer

@Suppress("unused")
class DataView(
    var ptr: Long,
    var size: Int
) {
    // A direct buffer keeps the storage off heap, so the address stays valid while this view is reachable.
    private var storage: ByteBuffer? = null

    companion object {
        fun createFromByteBuffer(data: ByteBuffer): DataView {
            // A heap buffer has no address native code can use, and getting zero back would point the view at nothing.

            if (!data.isDirect) {
                Log.e("[DataView : createFromByteBuffer] The buffer is not direct")
                return DataView(0, 0)
            }

            return DataView(ByteBufferHelper.getPtrAddress(data), data.capacity())
        }

        fun createFromByteArray(data: ByteArray): DataView {
            val storage = ByteBuffer.allocateDirect(data.size)
            storage.put(data)

            return DataView(ByteBufferHelper.getPtrAddress(storage), data.size).apply {
                this.storage = storage
            }
        }
    }
}
