package com.xplpc.system

internal object NativeLib {
    // Raises UnsatisfiedLinkError when the library is missing or its bridge to kotlin could not be resolved.
    fun initialize() {
        System.loadLibrary("xplpc")
    }
}
