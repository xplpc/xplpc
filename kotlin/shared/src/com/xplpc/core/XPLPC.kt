package com.xplpc.core

import com.xplpc.system.NativeLib
import com.xplpc.util.Log

object XPLPC {
    // A mapping may answer from a thread that never synchronized on this object, so both are published rather than merely assigned.
    @Volatile
    var initialized: Boolean = false
        private set

    @Volatile
    lateinit var config: Config
        private set

    @Synchronized
    fun initialize(config: Config) {
        if (initialized) {
            return
        }

        Log.d("[XPLPC : initialize]")

        this.config = config

        NativeLib.initialize()

        // This is only set once the native library is loaded, since a failure above must not leave a half built singleton.
        initialized = true
    }
}
