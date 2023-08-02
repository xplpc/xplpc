package com.xplpc.platform

import com.xplpc.core.Config
import com.xplpc.core.XPLPC
import com.xplpc.serializer.JsonSerializer

object PlatformInitializer {
    fun initialize() {
        val debug = System.getProperty("java.vm.debug") != null

        XPLPC.initialize(
            Config(debug, JsonSerializer()),
        )
    }
}
