package com.xplpc.client

import com.google.gson.reflect.TypeToken
import com.xplpc.core.XPLPC
import com.xplpc.data.CallbackList
import com.xplpc.message.Request
import com.xplpc.proxy.PlatformProxy
import com.xplpc.util.Log
import com.xplpc.util.UniqueID
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.suspendCancellableCoroutine
import kotlinx.coroutines.withContext
import kotlin.coroutines.resume
import kotlin.coroutines.resumeWithException

object Client {
    inline fun <reified T> call(request: Request, noinline callback: ((T?) -> Unit)?) {
        val key = UniqueID.generate()

        CallbackList.add(key) { response ->
            val type = object : TypeToken<T>() {}

            val data: T? = try {
                XPLPC.config.serializer.decodeFunctionReturnValue(response, type)
            } catch (e: Exception) {
                Log.e("[Client : call] Error when decode data: ${e.message}")
                null
            }

            callback?.invoke(data)
        }

        PlatformProxy.callNativeProxy(key, request.data)
    }

    inline fun <reified T> call(request: Request) {
        call<T>(request, null)
    }

    fun call(requestData: String, callback: ((String) -> Unit)?) {
        val key = UniqueID.generate()

        CallbackList.add(key) { response ->
            callback?.invoke(response)
        }

        PlatformProxy.callNativeProxy(key, requestData)
    }

    fun call(data: String) {
        call(data, null)
    }

    suspend inline fun <reified T> callAsync(request: Request): T? = withContext(Dispatchers.IO) {
        val key = UniqueID.generate()

        suspendCancellableCoroutine { continuation ->
            CallbackList.add(key) { response ->
                try {
                    val type = object : TypeToken<T>() {}
                    val data: T? = XPLPC.config.serializer.decodeFunctionReturnValue(response, type)
                    continuation.resume(data)
                } catch (e: Exception) {
                    Log.e("[Client : callAsync] Error when decode data: ${e.message}")
                    continuation.resumeWithException(e)
                }
            }

            PlatformProxy.callNativeProxy(key, request.data)
        }
    }

    suspend fun callAsync(requestData: String): String = withContext(Dispatchers.IO) {
        val key = UniqueID.generate()

        suspendCancellableCoroutine { continuation ->
            CallbackList.add(key) { response ->
                try {
                    continuation.resume(response)
                } catch (e: Exception) {
                    Log.e("[Client : callAsync] Error: ${e.message}")
                    continuation.resumeWithException(e)
                }
            }

            PlatformProxy.callNativeProxy(key, requestData)
        }
    }
}
