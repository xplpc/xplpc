package com.xplpc.library

import com.xplpc.data.MappingList
import com.xplpc.map.MappingItem
import com.xplpc.message.Param
import com.xplpc.message.Request
import kotlin.test.Test
import kotlin.test.assertEquals

// What a mapping reads out of a request, which is decided by the type it asks for rather than by a decode made before it asked.
class MessageTest : PendingGuard() {
    private fun setUp() {
        TestPlatform.initialize()
    }

    // A double holds integers exactly only up to two to the fifty third, so a mapping has to read the literal rather than a number decoded in advance.
    @Test
    fun aHostMappingReadsAnIntegerExactlyAsTheTypeItAsksFor() {
        setUp()

        MappingList.add(
            "platform.exact.integer",
            MappingItem { data, r ->
                val asLong = data.get<Long>("id")
                val asInt = data.get<Int>("count")
                val asDouble = data.get<Double>("count")
                val asBoolean = data.get<Boolean>("count")

                r("$asLong/$asInt/$asDouble/$asBoolean")
            }
        )

        val request =
            Request(
                "platform.exact.integer",
                Param("id", 9_007_199_254_740_993L),
                Param("count", 42)
            )

        verifyCall<String>(request) { response ->
            assertEquals("9007199254740993/42/42.0/null", response)
        }
    }
}
