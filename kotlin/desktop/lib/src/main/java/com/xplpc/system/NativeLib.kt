package com.xplpc.system

import com.xplpc.util.Log
import java.io.File
import java.io.IOException
import java.nio.file.Files

internal object NativeLib {
    // Raises UnsatisfiedLinkError when the library cannot be extracted, is missing or its bridge to kotlin could not be resolved.
    fun initialize() {
        val libFile =
            try {
                extract()
            } catch (e: IOException) {
                throw UnsatisfiedLinkError("The native library could not be extracted").apply { initCause(e) }
            }

        System.load(libFile.absolutePath)

        Log.d("[NativeLib : initialize] Native library loaded from: ${libFile.absolutePath}")

        // A loaded library stays mapped once its file is gone on linux and macos, and windows refuses until the process ends, which the exit hook then tries again.
        libFile.delete()
        libFile.parentFile.delete()
    }

    // A directory of its own per extraction means no other process can replace the file between writing it and loading it.
    fun extract(): File {
        val libFileName = System.mapLibraryName("xplpc")

        val input =
            NativeLib::class.java.getResourceAsStream("/native-libs/$libFileName")
                ?: throw UnsatisfiedLinkError("The native library $libFileName is not in the classpath")

        val dir = Files.createTempDirectory("xplpc-").toFile()
        val libFile = File(dir, libFileName)

        input.use { source ->
            libFile.outputStream().use { target -> source.copyTo(target) }
        }

        dir.deleteOnExit()
        libFile.deleteOnExit()

        return libFile
    }
}
