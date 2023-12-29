package com.xplpc.system

import com.xplpc.util.Log
import java.io.File
import java.io.FileOutputStream

object NativeLib {
    fun initialize() {
        Log.d("[NativeLib : initialize] Loading XPLPC native library...")

        try {
            val tempDir = System.getProperty("java.io.tmpdir")
            val libFileName = System.mapLibraryName("xplpc")
            val libFile = File(tempDir, libFileName)

            if (libFile.exists()) {
                libFile.delete()
            }

            val inputStream =
                NativeLib::class.java.getResourceAsStream("/native-libs/$libFileName")
                    ?: throw UnsatisfiedLinkError("Library '$libFileName' not found in the classpath.")

            FileOutputStream(libFile).use { output ->
                inputStream.copyTo(output)
                Log.d("[NativeLib : initialize] XPLPC native library copied to: $output")
            }

            System.load(libFile.absolutePath)

            Log.d("[NativeLib : initialize] XPLPC native library loaded")
        } catch (e: UnsatisfiedLinkError) {
            Log.e("[NativeLib : initialize] Could not load XPLPC native library: " + e.message)
        }
    }
}
