package com.xplpc.layer

import com.xplpc.core.XPLPC
import com.xplpc.data.MappingList
import com.xplpc.data.PendingCall
import com.xplpc.message.Message
import com.xplpc.util.Log
import java.nio.ByteBuffer

// Kotlin as one layer of the router, attached when the native library loads.
@PublishedApi
internal object Layer {
    @JvmStatic
    external fun declare(name: String)

    @JvmStatic
    external fun retract(name: String)

    // Answers the handle of a call still waiting, or zero once the pending call was answered, which happens before this returns when the mapping answered inline.
    @JvmStatic
    external fun call(
        request: String,
        pendingCall: PendingCall
    ): Long

    @JvmStatic
    external fun cancel(call: Long): Boolean

    // The number of calls still waiting anywhere in the process, which is what lets a suite prove it left none behind.
    @JvmStatic
    external fun pending(): Long

    @JvmStatic
    external fun answer(
        call: Long,
        response: String
    )

    // Answers the handle of a direct buffer the native side now holds until it is released, or zero when the buffer could not be registered.
    @JvmStatic
    external fun registerBuffer(data: ByteBuffer): Long

    // Answers false for a buffer kotlin did not register or already released, since only the layer that registered a buffer may release it.
    @JvmStatic
    external fun releaseBuffer(buffer: Long): Boolean

    // Answers a copy of the bytes while their owner keeps them registered, and null for a buffer that was released or never registered.
    @JvmStatic
    external fun readBuffer(buffer: Long): ByteArray?

    @JvmStatic
    fun onInvoke(
        call: Long,
        data: String
    ) {
        // A name is declared only once the library loaded, and the config is published after that, so a call can arrive in between.
        if (!XPLPC.initialized) {
            Log.e("[Layer : invoke] XPLPC was not initialized")
            answer(call, "")
            return
        }

        // A serializer of the consumer's own can raise while it reads the request, which is reported the way every other bridge reports it.
        val request =
            try {
                XPLPC.config.serializer.decodeRequest(data)
            } catch (e: Throwable) {
                Log.e("[Layer : invoke] Error when decode request")
                Log.d("[Layer : invoke] Error when decode request: ${e.message}")
                null
            }

        if (request == null) {
            answer(call, "")
            return
        }

        val mappingItem = MappingList.find(request.functionName)

        if (mappingItem == null) {
            Log.e("[Layer : invoke] Mapping not found for function: ${request.functionName}")
            answer(call, "")
            return
        }

        execute(call, request.functionName, mappingItem.target, request.message)
    }

    private fun execute(
        call: Long,
        functionName: String,
        target: (Message, (Any?) -> Unit) -> Unit,
        message: Message,
    ) {
        try {
            target(message) { response ->
                val encodedData =
                    try {
                        XPLPC.config.serializer.encodeFunctionReturnValue(response)
                    } catch (e: Throwable) {
                        // A mapping may answer from a thread of its own, where an error leaving this block ends that thread with the caller never answered.
                        Log.e("[Layer : invoke] Error when encode the answer of function \"$functionName\"")
                        Log.d("[Layer : invoke] Error when encode the answer of function \"$functionName\": ${e.message}")
                        ""
                    }

                answer(call, encodedData)
            }
        } catch (e: Throwable) {
            // An error is not an exception here, and letting one leave this frame makes it a pending exception at the jni boundary.
            Log.e("[Layer : invoke] Error when execute function \"$functionName\"")
            Log.d("[Layer : invoke] Error when execute function \"$functionName\": ${e.message}")
            answer(call, "")
        }
    }
}
