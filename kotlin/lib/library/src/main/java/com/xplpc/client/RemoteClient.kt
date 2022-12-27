package com.xplpc.client

import com.google.gson.reflect.TypeToken
import com.xplpc.core.XPLPC
import com.xplpc.message.Request
import com.xplpc.proxy.PlatformProxy
import com.xplpc.util.Log
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Deferred
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.async

object RemoteClient {
    inline fun <reified T> call(request: Request, defValue: T? = null): T? {
        try {
            val data = PlatformProxy.doProxyCall(request.data)
            val type = object : TypeToken<T>() {}
            return XPLPC.config.serializer.decodeFunctionReturnValue(data, type)
        } catch (e: Exception) {
            Log.e("[RemoteClient : call] Error when try to decode return value: ${e.message}")
        }

        return defValue
    }

    inline fun <reified T> callAsync(request: Request, defValue: T? = null): Deferred<T?> {
        return CoroutineScope(Dispatchers.IO).async {
            return@async call<T>(request, defValue)
        }
    }
}
