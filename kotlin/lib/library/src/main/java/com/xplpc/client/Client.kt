package com.xplpc.client

import com.google.gson.reflect.TypeToken
import com.xplpc.core.XPLPC
import com.xplpc.data.CallbackList
import com.xplpc.message.Request
import com.xplpc.proxy.PlatformProxy
import com.xplpc.util.Log
import com.xplpc.util.UniqueID

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
}
