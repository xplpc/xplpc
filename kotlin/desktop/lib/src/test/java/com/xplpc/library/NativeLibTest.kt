package com.xplpc.library

import com.xplpc.system.NativeLib
import java.io.File
import java.nio.file.Files
import java.util.concurrent.TimeUnit
import javax.tools.ToolProvider
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFalse
import kotlin.test.assertNotEquals
import kotlin.test.assertNotNull
import kotlin.test.assertTrue

// The loader is the one piece of the desktop target that android does not share, since android loads by name out of the package.
class NativeLibTest {
    @Test
    fun everyExtractionGetsAPathNoOtherProcessShares() {
        val first = NativeLib.extract()
        val second = NativeLib.extract()

        assertNotEquals(first.parentFile, second.parentFile)
        assertNotEquals(File(System.getProperty("java.io.tmpdir"), first.name).canonicalFile, first.canonicalFile)
        assertTrue(first.length() > 0)
        assertEquals(first.length(), second.length())
    }

    // A library that did not load has to leave xplpc down and able to try again, which only a process that never loaded it can show.
    @Test
    fun aLibraryThatDoesNotLoadLeavesTheLibraryDownAndRetryable() {
        val classPath =
            System
                .getProperty("java.class.path")
                .split(File.pathSeparator)
                .filterNot { File(it, "native-libs").exists() }
                .joinToString(File.pathSeparator)

        val java = File(System.getProperty("java.home"), "bin/java").path
        val process =
            ProcessBuilder(java, "-cp", classPath, FailedLoadProbe::class.java.name)
                .redirectErrorStream(true)
                .start()

        assertTrue(process.waitFor(60, TimeUnit.SECONDS))

        val output = process.inputStream.bufferedReader().readText()

        assertTrue(output.contains("first=false initialized=false second=false"), output)
    }

    // A release build ships whatever kotlin side its keep rules left, so the library has to refuse one that lacks what the bridge resolves without making a lookup while an exception is pending.
    @Test
    fun aLibraryWhoseKotlinSideDoesNotResolveIsRefusedWithNoLookupMadeOverAPendingException() {
        val library = NativeLib.extract()

        val withoutKotlin = Files.createTempDirectory("xplpc-stubs-").toFile()

        val withoutOnInvoke =
            compileStubs(
                "com/xplpc/layer/Layer.java" to "package com.xplpc.layer; public class Layer {}",
                "com/xplpc/data/PendingCall.java" to
                    "package com.xplpc.data; public class PendingCall { public void onAnswer(String response) {} }"
            )

        for (stubs in listOf(withoutKotlin, withoutOnInvoke)) {
            val output = loadInAnotherProcess(library, stubs)

            assertTrue(output.contains("refused"), output)
            assertFalse(output.contains("exception pending"), output)
        }
    }

    private fun compileStubs(vararg sources: Pair<String, String>): File {
        val sourceDir = Files.createTempDirectory("xplpc-stub-sources-").toFile()
        val classDir = Files.createTempDirectory("xplpc-stubs-").toFile()

        val files =
            sources.map { (path, code) ->
                val file = File(sourceDir, path)
                file.parentFile.mkdirs()
                file.writeText(code)
                file.path
            }

        val compiler = assertNotNull(ToolProvider.getSystemJavaCompiler(), "the suite runs on a jdk")

        assertEquals(0, compiler.run(null, null, null, "-d", classDir.path, *files.toTypedArray()))

        return classDir
    }

    // The library classes are left out of the classpath, so the stubs are the only kotlin side the bridge can find.
    private fun loadInAnotherProcess(
        library: File,
        stubs: File
    ): String {
        val classPath =
            System
                .getProperty("java.class.path")
                .split(File.pathSeparator)
                .filterNot { File(it, "com/xplpc/layer/Layer.class").exists() }
                .plus(stubs.path)
                .joinToString(File.pathSeparator)

        val java = File(System.getProperty("java.home"), "bin/java").path
        val process =
            ProcessBuilder(java, "-Xcheck:jni", "-cp", classPath, LoadProbe::class.java.name, library.path)
                .redirectErrorStream(true)
                .start()

        assertTrue(process.waitFor(60, TimeUnit.SECONDS))

        return process.inputStream.bufferedReader().readText()
    }
}
