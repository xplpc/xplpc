package com.xplpc.platform

import com.xplpc.core.Config
import com.xplpc.core.XPLPC
import com.xplpc.serializer.JsonSerializer
import com.xplpc.util.Log

object PlatformInitializer {
    fun initialize() {
        // The debug level is raised by the same variable the native side reads, so one setting covers the whole stack.
        val level = System.getenv("XPLPC_LOG_LEVEL")
        val debug = level == "debug" || level == "trace"

        Log.isEnabled = debug

        XPLPC.initialize(
            Config(JsonSerializer()),
        )
    }
}
