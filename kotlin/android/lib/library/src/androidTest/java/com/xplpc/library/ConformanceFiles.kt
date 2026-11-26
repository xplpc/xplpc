package com.xplpc.library

import androidx.test.platform.app.InstrumentationRegistry

// Opens the shared conformance cases from the assets of the test package, which is how the build ships them to the device.
object ConformanceFiles {
    private val assets get() = InstrumentationRegistry.getInstrumentation().context.assets

    fun names(group: String): List<String> =
        assets
            .list(group)
            .orEmpty()
            .filter { it.endsWith(".json") }
            .map { it.removeSuffix(".json") }
            .sorted()

    fun read(
        group: String,
        name: String
    ): String = assets.open("$group/$name.json").bufferedReader().use { it.readText() }
}
