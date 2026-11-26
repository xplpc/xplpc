package com.xplpc.library

import com.xplpc.core.Config
import com.xplpc.core.XPLPC
import com.xplpc.serializer.JsonSerializer

// Runs in a process whose classpath carries no native library, so loading it fails the way a missing binary does.
object FailedLoadProbe {
    @JvmStatic
    fun main(args: Array<String>) {
        val first = XPLPC.initialize(Config(JsonSerializer()))
        val initialized = XPLPC.initialized
        val second = XPLPC.initialize(Config(JsonSerializer()))

        println("first=$first initialized=$initialized second=$second")
    }
}
