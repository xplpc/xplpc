package com.xplpc.library

// Loads the native library in a process whose classpath carries whatever kotlin side the test arranged, and says whether the library accepted it.
object LoadProbe {
    @JvmStatic
    fun main(args: Array<String>) {
        try {
            System.load(args[0])
            println("loaded")
        } catch (e: UnsatisfiedLinkError) {
            println("refused: ${e.message}")
        }
    }
}
