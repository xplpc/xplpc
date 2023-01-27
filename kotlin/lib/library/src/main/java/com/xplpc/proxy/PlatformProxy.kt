package com.xplpc.proxy

import com.xplpc.client.ProxyClient
import com.xplpc.data.CallbackList

class PlatformProxy {
    companion object {
        @JvmStatic
        external fun nativeProxyCall(key: String, data: String)

        @JvmStatic
        external fun nativeCallProxyCallback(key: String, data: String)

        @JvmStatic
        fun callProxyCallback(key: String, data: String) {
            CallbackList.execute(key, data)
        }

        @JvmStatic
        fun onNativeProxyCall(key: String, data: String) {
            ProxyClient.call(data) { response ->
                nativeCallProxyCallback(key, response)
            }
        }
    }
}
