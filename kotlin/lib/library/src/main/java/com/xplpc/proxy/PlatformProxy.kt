package com.xplpc.proxy

import com.xplpc.core.XPLPC
import com.xplpc.data.CallbackList
import com.xplpc.data.MappingList
import com.xplpc.message.Message
import com.xplpc.util.Log

class PlatformProxy {
    companion object {
        @JvmStatic
        external fun nativeProxyCall(key: String, data: String)

        @JvmStatic
        external fun nativeCallProxyCallback(key: String, data: String)

        @JvmStatic
        fun onCallProxyCallback(key: String, data: String) {
            CallbackList.execute(key, data)
        }

        @JvmStatic
        fun onNativeProxyCall(key: String, data: String) {
            // function name
            val functionName = XPLPC.config.serializer.decodeFunctionName(data)

            if (functionName.isEmpty()) {
                Log.e("[PlatformProxy : call] Function name is empty")
                nativeCallProxyCallback(key, data)
                return
            }

            // mapping item
            val mappingItem = MappingList.find(functionName)

            if (mappingItem == null) {
                Log.e("[PlatformProxy : call] Mapping not found for function: $functionName")
                nativeCallProxyCallback(key, data)
                return
            }

            // message
            var message: Message? = null

            try {
                message = XPLPC.config.serializer.decodeMessage(data)
            } catch (e: Exception) {
                Log.e("[PlatformProxy : call] Error when decode message: ${e.message}")
            }

            if (message == null) {
                Log.e("[PlatformProxy : call] Error when decode message for function: $functionName")
                nativeCallProxyCallback(key, data)
                return
            }

            // execute
            mappingItem.target(message) { response ->
                val encodedData: String?

                try {
                    encodedData = XPLPC.config.serializer.encodeFunctionReturnValue(response)
                } catch (e: Exception) {
                    Log.e("[PlatformProxy : call] Error when encode data: ${e.message}")
                    nativeCallProxyCallback(key, data)
                    return@target
                }

                nativeCallProxyCallback(key, encodedData)
            }
        }
    }
}
