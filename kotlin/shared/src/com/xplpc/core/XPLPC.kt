package com.xplpc.core

import com.xplpc.system.NativeLib
import com.xplpc.util.Log

object XPLPC {
    // A mapping may answer from a thread that never synchronized on this object, so both are published rather than merely assigned.
    @Volatile
    var initialized: Boolean = false
        private set

    @Volatile
    @PublishedApi
    internal lateinit var config: Config
        private set

    // Answers whether the library is up, and a failure publishes nothing so a later call can try again.
    @Synchronized
    fun initialize(config: Config): Boolean {
        if (initialized) {
            return true
        }

        Log.d("[XPLPC : initialize]")

        try {
            NativeLib.initialize()
        } catch (e: UnsatisfiedLinkError) {
            Log.e("[XPLPC : initialize] Could not load the native library")
            Log.d("[XPLPC : initialize] Could not load the native library: ${e.message}")
            return false
        }

        this.config = config

        // This says the library can serve a call, so it is the last step of bringing it up.
        initialized = true

        return true
    }
}
