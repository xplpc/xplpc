package com.xplpc.library

import java.io.File

// Opens the shared conformance cases where the repository keeps them, which the build passes in since the jvm runs from the project directory.
object ConformanceFiles {
    private val root = File(System.getProperty("xplpc.conformance"))

    fun names(group: String): List<String> =
        root
            .resolve(group)
            .listFiles { file -> file.extension == "json" }
            .orEmpty()
            .map { it.nameWithoutExtension }
            .sorted()

    fun read(
        group: String,
        name: String
    ): String = root.resolve(group).resolve("$name.json").readText()
}
