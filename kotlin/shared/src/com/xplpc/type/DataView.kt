package com.xplpc.type

import com.xplpc.layer.Layer
import com.xplpc.util.Log
import java.nio.ByteBuffer
import java.util.concurrent.atomic.AtomicBoolean

// Names a buffer registered with the native side, which is all that crosses the wire, so a reader is never pointed at memory nobody registered.
class DataView internal constructor(
    val buffer: Long,
    registered: Boolean
) {
    // Only the view that registered the buffer may release it, and only once.
    @Transient
    private val owned = AtomicBoolean(registered)

    // Lets go of a buffer this view registered, after which every reader of it reads nothing, and answers false for a view that did not register its buffer or already released it.
    fun release(): Boolean {
        if (!owned.getAndSet(false)) {
            return false
        }

        return Layer.releaseBuffer(buffer)
    }

    companion object {
        // Registers a direct buffer, which the native side keeps alive and in place until the view is released, so forgetting to release it leaks the buffer rather than freeing it under a reader.
        fun createFromByteBuffer(data: ByteBuffer): DataView {
            // A heap buffer has no address native code can hold, so the view names nothing rather than a buffer that may move.

            if (!data.isDirect) {
                Log.e("[DataView : createFromByteBuffer] The buffer is not direct")
                return DataView(0, false)
            }

            return register(data)
        }

        // Copies the array into a direct buffer and registers it, since a managed heap moves its arrays.
        fun createFromByteArray(data: ByteArray): DataView {
            val storage = ByteBuffer.allocateDirect(data.size)
            storage.put(data)

            return register(storage)
        }

        private fun register(data: ByteBuffer): DataView {
            val buffer = Layer.registerBuffer(data)
            return DataView(buffer, buffer != 0L)
        }
    }
}
