package com.xplpc.client

import com.google.gson.reflect.TypeToken
import com.xplpc.core.XPLPC
import com.xplpc.data.PendingCall
import com.xplpc.layer.Layer
import com.xplpc.message.Request
import com.xplpc.serializer.Nullability
import com.xplpc.util.Log
import kotlinx.coroutines.suspendCancellableCoroutine
import kotlinx.coroutines.yield
import java.util.concurrent.atomic.AtomicReference
import kotlin.coroutines.resume
import kotlin.reflect.typeOf

object Client {
    inline fun <reified T> call(
        request: Request,
        noinline callback: ((T?) -> Unit)?
    ) {
        Layer.call(request.data, PendingCall { response -> callback?.invoke(decode<T>(response, "call")) })
    }

    inline fun <reified T> call(request: Request) {
        call<T>(request, null)
    }

    fun call(
        requestData: String,
        callback: ((String) -> Unit)?
    ) {
        Layer.call(requestData, PendingCall { response -> callback?.invoke(response) })
    }

    fun call(data: String) {
        call(data, null)
    }

    inline fun <reified T> callSync(request: Request): T? {
        val response = answerSynchronously(request.data) ?: return null
        return decode<T>(response, "callSync")
    }

    fun callSync(requestData: String): String = answerSynchronously(requestData) ?: ""

    suspend inline fun <reified T> callAsync(request: Request): T? {
        val response = answerAsynchronously(request.data)
        return decode<T>(response, "callAsync")
    }

    suspend fun callAsync(requestData: String): String = answerAsynchronously(requestData)

    @PublishedApi
    internal inline fun <reified T> decode(
        response: String,
        entryPoint: String
    ): T? {
        val type = object : TypeToken<T>() {}

        val value =
            try {
                XPLPC.config.serializer.decodeFunctionReturnValue(response, type)
            } catch (e: Throwable) {
                Log.e("[Client : $entryPoint] Error when decode data")
                Log.d("[Client : $entryPoint] Error when decode data: ${e.message}")
                return null
            }

        // The jvm erases what a type says about null inside a list or a map, so a value holding null where its type holds none is refused here.
        if (!Nullability.holds(value, typeOf<T?>())) {
            Log.e("[Client : $entryPoint] Value holds null where its type holds none")
            return null
        }

        return value
    }

    // A mapping that had not answered when the call returned is cancelled, so its later answer resolves nothing and leaves nothing behind.
    @PublishedApi
    internal fun answerSynchronously(data: String): String? {
        val answer = AtomicReference<String?>(null)
        val call = Layer.call(data, PendingCall { response -> answer.set(response) })

        if (call == 0L) {
            return answer.get()
        }

        Layer.cancel(call)
        Log.e("[Client : callSync] The function did not answer synchronously")

        return null
    }

    @PublishedApi
    internal suspend fun answerAsynchronously(data: String): String {
        // The scheduler gets a turn before the call is made, since resuming inside the block below would not suspend at all.
        yield()

        return suspendCancellableCoroutine { continuation ->
            if (!continuation.isActive) {
                return@suspendCancellableCoroutine
            }

            val pendingCall =
                PendingCall { response ->
                    if (continuation.isActive) {
                        continuation.resume(response)
                    }
                }

            val call = Layer.call(data, pendingCall)

            if (call != 0L) {
                continuation.invokeOnCancellation { Layer.cancel(call) }
            }
        }
    }
}
