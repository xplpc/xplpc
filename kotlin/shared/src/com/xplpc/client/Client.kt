package com.xplpc.client

import com.google.gson.reflect.TypeToken
import com.xplpc.core.XPLPC
import com.xplpc.data.CallbackList
import com.xplpc.message.Request
import com.xplpc.proxy.PlatformProxy
import com.xplpc.util.Log
import com.xplpc.util.UniqueID
import kotlinx.coroutines.suspendCancellableCoroutine
import kotlinx.coroutines.yield
import java.util.concurrent.atomic.AtomicReference
import kotlin.coroutines.resume

object Client {
    // The key is taken before the native side is reached, so failing to reach it has to drop the key as well as raise.
    @PublishedApi
    internal fun dispatch(key: String, data: String) {
        try {
            PlatformProxy.callNativeProxy(key, data)
        } catch (e: Throwable) {
            CallbackList.remove(key)
            throw e
        }
    }

    inline fun <reified T> call(request: Request, noinline callback: ((T?) -> Unit)?) {
        val key = UniqueID.generate()

        CallbackList.add(key) { response ->
            val type = object : TypeToken<T>() {}

            val data: T? = try {
                XPLPC.config.serializer.decodeFunctionReturnValue(response, type)
            } catch (e: Exception) {
                Log.e("[Client : call] Error when decode data")
                Log.d("[Client : call] Error when decode data: ${e.message}")
                null
            }

            callback?.invoke(data)
        }

        dispatch(key, request.data)
    }

    inline fun <reified T> call(request: Request) {
        call<T>(request, null)
    }

    fun call(requestData: String, callback: ((String) -> Unit)?) {
        val key = UniqueID.generate()

        CallbackList.add(key) { response ->
            callback?.invoke(response)
        }

        dispatch(key, requestData)
    }

    fun call(data: String) {
        call(data, null)
    }

    inline fun <reified T> callSync(request: Request): T? {
        val response = answerSynchronously(request.data) ?: return null

        val type = object : TypeToken<T>() {}

        return try {
            XPLPC.config.serializer.decodeFunctionReturnValue(response, type)
        } catch (e: Exception) {
            Log.e("[Client : callSync] Error when decode data")
            Log.d("[Client : callSync] Error when decode data: ${e.message}")
            null
        }
    }

    fun callSync(requestData: String): String {
        return answerSynchronously(requestData) ?: ""
    }

    // A mapping is free to answer from a thread of its own after this function has returned, so what it writes into outlives the frame and is published.
    @PublishedApi
    internal fun answerSynchronously(data: String): String? {
        val answer = AtomicReference<String?>(null)
        val key = UniqueID.generate()

        CallbackList.add(key) { response -> answer.set(response) }

        dispatch(key, data)

        // Taking the key back is what decides the two cases, since a mapping that answered inline has already taken it and one that deferred never will.
        CallbackList.remove(key)

        val response = answer.get()

        if (response == null) {
            Log.e("[Client : callSync] The function did not answer synchronously")
        }

        return response
    }

    suspend inline fun <reified T> callAsync(request: Request): T? {
        yield()

        val key = UniqueID.generate()

        return suspendCancellableCoroutine { continuation ->
            continuation.invokeOnCancellation { CallbackList.remove(key) }

            CallbackList.add(key) { response ->
                if (!continuation.isActive) {
                    return@add
                }

                val type = object : TypeToken<T>() {}

                val data: T? = try {
                    XPLPC.config.serializer.decodeFunctionReturnValue(response, type)
                } catch (e: Exception) {
                    Log.e("[Client : callAsync] Error when decode data")
                    Log.d("[Client : callAsync] Error when decode data: ${e.message}")
                    null
                }

                continuation.resume(data)
            }

            dispatch(key, request.data)
        }
    }

    suspend fun callAsync(requestData: String): String {
        yield()

        val key = UniqueID.generate()

        return suspendCancellableCoroutine { continuation ->
            continuation.invokeOnCancellation { CallbackList.remove(key) }

            CallbackList.add(key) { response ->
                if (continuation.isActive) {
                    continuation.resume(response)
                }
            }

            dispatch(key, requestData)
        }
    }
}
