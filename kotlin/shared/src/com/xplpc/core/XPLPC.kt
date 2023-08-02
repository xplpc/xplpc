package com.xplpc.core

import com.xplpc.system.NativeLib
import com.xplpc.util.Log

object XPLPC {
    var initialized: Boolean = false
        private set

    lateinit var config: Config
        private set

    @Synchronized
    fun initialize(config: Config) {
        Log.isEnabled = config.debug

        Log.d("[XPLPC : initialize]")

        if (initialized) {
            return
        }

        initialized = true
        this.config = config

        // load native library
        NativeLib.initialize()
    }
}
