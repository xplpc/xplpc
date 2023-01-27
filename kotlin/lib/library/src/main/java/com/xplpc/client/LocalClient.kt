package com.xplpc.client

import com.xplpc.core.XPLPC
import com.xplpc.data.MappingList
import com.xplpc.message.Message
import com.xplpc.message.Request
import com.xplpc.util.Log

object LocalClient {
    inline fun <reified T> call(request: Request, noinline callback: ((T?) -> Unit)?) {
        val data = request.data

        // function name
        val functionName = XPLPC.config.serializer.decodeFunctionName(data)

        if (functionName.isEmpty()) {
            Log.e("[LocalClient : call] Function name is empty")
            callback?.invoke(null)
            return
        }

        // mapping item
        val mappingItem = MappingList.find(functionName)

        if (mappingItem == null) {
            Log.e("[LocalClient : call] Mapping not found for function: $functionName")
            callback?.invoke(null)
            return
        }

        // execute
        var message: Message? = null

        try {
            message = XPLPC.config.serializer.decodeMessage(data)
        } catch (e: Exception) {
            Log.e("[LocalClient : call] Error when decode message: ${e.message}")
        }

        if (message == null) {
            Log.e("[LocalClient : call] Error when decode message for function: $functionName")
            callback?.invoke(null)
            return
        }

        mappingItem.target(message) { response ->
            callback?.invoke(response as? T)
        }
    }

    inline fun <reified T> call(request: Request) {
        call<T>(request, null)
    }
}
