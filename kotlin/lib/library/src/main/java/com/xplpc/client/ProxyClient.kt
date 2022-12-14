package com.xplpc.client

import com.xplpc.core.XPLPC
import com.xplpc.data.MappingData
import com.xplpc.util.Log
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Deferred
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.async

object ProxyClient {
    fun call(data: String): String {
        // function name
        val functionName = XPLPC.config.serializer.decodeFunctionName(data)

        if (functionName.isEmpty()) {
            Log.e("[ProxyClient : call] Function name is empty")
            return ""
        }

        // mapping item
        val mappingItem = MappingData.find(functionName)

        if (mappingItem == null) {
            Log.e("[ProxyClient : call] Mapping not found for function: $functionName")
            return ""
        }

        // execute
        try {
            val message = XPLPC.config.serializer.decodeMessage(data)

            if (message == null) {
                Log.e("[ProxyClient : call] Error when try to decode message data for function: $functionName")
                return ""
            }

            return try {
                return XPLPC.config.serializer.encodeFunctionReturnValue(
                    mappingItem.target(message).value,
                )
            } catch (e: Exception) {
                Log.e("[ProxyClient : call] Error when try to encode return value for function: $functionName")
                ""
            }
        } catch (e: Exception) {
            Log.e("[ProxyClient : call] Error when try to encode return value: ${e.message}")
        }

        return ""
    }

    fun callAsync(data: String): Deferred<String> {
        return CoroutineScope(Dispatchers.IO).async {
            return@async call(data)
        }
    }
}
