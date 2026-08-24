package com.xplpc.proxy

import com.xplpc.core.XPLPC
import com.xplpc.data.CallbackList
import com.xplpc.data.MappingList
import com.xplpc.message.Message
import com.xplpc.util.Log

class PlatformProxy private constructor() {
    companion object {
        @JvmStatic
        external fun callNativeProxy(key: String, data: String)

        @JvmStatic
        external fun callNativeProxyCallback(key: String, data: String)

        @JvmStatic
        fun onNativeProxyCallback(key: String, data: String) {
            CallbackList.execute(key, data)
        }

        @JvmStatic
        fun onNativeProxyCall(key: String, data: String) {
            if (!XPLPC.initialized) {
                Log.e("[PlatformProxy : call] XPLPC was not initialized")
                callNativeProxyCallback(key, "")
                return
            }

            val request = XPLPC.config.serializer.decodeRequest(data)

            if (request == null) {
                callNativeProxyCallback(key, "")
                return
            }

            if (request.functionName.isEmpty()) {
                Log.e("[PlatformProxy : call] Function name is empty")
                callNativeProxyCallback(key, "")
                return
            }

            val mappingItem = MappingList.find(request.functionName)

            if (mappingItem == null) {
                Log.e("[PlatformProxy : call] Mapping not found for function: ${request.functionName}")
                callNativeProxyCallback(key, "")
                return
            }

            execute(key, request.functionName, mappingItem.target, request.message)
        }

        @JvmStatic
        fun onHasMapping(name: String): Boolean {
            return MappingList.has(name)
        }

        @JvmStatic
        fun onInitializePlatform() {
        }

        @JvmStatic
        fun onFinalizePlatform() {
            MappingList.clear()
        }

        private fun execute(
            key: String,
            functionName: String,
            target: (Message, (Any?) -> Unit) -> Unit,
            message: Message,
        ) {
            try {
                target(message) { response ->
                    val encodedData = try {
                        XPLPC.config.serializer.encodeFunctionReturnValue(response)
                    } catch (e: Exception) {
                        Log.e("[PlatformProxy : call] Error when encode data")
                        Log.d("[PlatformProxy : call] Error when encode data: ${e.message}")
                        ""
                    }

                    callNativeProxyCallback(key, encodedData)
                }
            } catch (e: Throwable) {
                // An error is not an exception here, and letting one leave this frame makes it a pending exception at the jni boundary with the caller never answered.
                Log.e("[PlatformProxy : call] Error when execute function \"$functionName\"")
                Log.d("[PlatformProxy : call] Error when execute function \"$functionName\": ${e.message}")
                callNativeProxyCallback(key, "")
            }
        }
    }
}
