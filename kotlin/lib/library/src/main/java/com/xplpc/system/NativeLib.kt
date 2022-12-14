package com.xplpc.system

import com.xplpc.util.Log

object NativeLib {
    fun initialize() {
        Log.d("[NativeLib : initialize] Loading XPLPC native library...")

        try {
            System.loadLibrary("xplpc")
            Log.d("[NativeLib : initialize] XPLPC native library loaded")
        } catch (e: UnsatisfiedLinkError) {
            Log.e("[NativeLib : initialize] Could not load XPLPC native library: " + e.message)
        }
    }
}
