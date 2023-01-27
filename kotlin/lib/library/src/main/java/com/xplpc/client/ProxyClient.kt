package com.xplpc.client

import com.xplpc.core.XPLPC
import com.xplpc.data.MappingList
import com.xplpc.message.Message
import com.xplpc.util.Log

object ProxyClient {
    fun call(data: String, callback: ((String) -> Unit)?) {
        // function name
        val functionName = XPLPC.config.serializer.decodeFunctionName(data)

        if (functionName.isEmpty()) {
            Log.e("[ProxyClient : call] Function name is empty")
            callback?.invoke("")
            return
        }

        // mapping item
        val mappingItem = MappingList.find(functionName)

        if (mappingItem == null) {
            Log.e("[ProxyClient : call] Mapping not found for function: $functionName")
            callback?.invoke("")
            return
        }

        // message
        var message: Message? = null

        try {
            message = XPLPC.config.serializer.decodeMessage(data)
        } catch (e: Exception) {
            Log.e("[ProxyClient : call] Error when decode message: ${e.message}")
        }

        if (message == null) {
            Log.e("[ProxyClient : call] Error when decode message for function: $functionName")
            callback?.invoke("")
            return
        }

        // execute
        mappingItem.target(message) { response ->
            val encodedData: String?

            try {
                encodedData = XPLPC.config.serializer.encodeFunctionReturnValue(response)
            } catch (e: Exception) {
                Log.e("[ProxyClient : call] Error when encode data: ${e.message}")
                callback?.invoke("")
                return@target
            }

            callback?.invoke(encodedData)
        }
    }

    fun call(data: String) {
        call(data, null)
    }
}
