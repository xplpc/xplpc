package com.xplpc.client

import com.xplpc.core.XPLPC
import com.xplpc.data.MappingData
import com.xplpc.message.Request
import com.xplpc.util.Log
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Deferred
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.async

object LocalClient {
    inline fun <reified T> call(request: Request, defValue: T? = null): T? {
        val data = request.data

        // function name
        val functionName = XPLPC.config.serializer.decodeFunctionName(data)

        if (functionName.isEmpty()) {
            Log.e("[LocalClient : call] Function name is empty")
            return defValue
        }

        // mapping item
        val mappingItem = MappingData.find(functionName)

        if (mappingItem == null) {
            Log.e("[LocalClient : call] Mapping not found for function: $functionName")
            return defValue
        }

        // execute
        try {
            val message = XPLPC.config.serializer.decodeMessage(data)

            if (message == null) {
                Log.e("[LocalClient : call] Error when try to decode message data for function: $functionName")
                return defValue
            }

            return try {
                return mappingItem.target(message).value as T
            } catch (e: Exception) {
                Log.e("[LocalClient : call] Error when try to decode return value for function: $functionName")

                defValue
            }
        } catch (e: Exception) {
            Log.e("[LocalClient : call] Error when try to decode return value: ${e.message}")
        }

        return defValue
    }

    inline fun <reified T> callAsync(request: Request, defValue: T? = null): Deferred<T?> {
        return CoroutineScope(Dispatchers.IO).async {
            return@async call<T>(request, defValue)
        }
    }
}
