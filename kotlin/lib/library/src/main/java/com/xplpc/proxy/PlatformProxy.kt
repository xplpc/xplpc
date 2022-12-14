package com.xplpc.proxy

import com.xplpc.client.ProxyClient

@Suppress("UtilityClassWithPublicConstructor")
class PlatformProxy {
    companion object {
        @JvmStatic
        external fun call(data: String): String

        @JvmStatic
        fun onRemoteClientCall(data: String): String {
            return ProxyClient.call(data)
        }
    }
}
