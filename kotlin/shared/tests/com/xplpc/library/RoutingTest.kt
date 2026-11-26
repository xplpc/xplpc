package com.xplpc.library

import com.xplpc.client.Client
import com.xplpc.data.MappingList
import com.xplpc.layer.Layer
import com.xplpc.map.MappingItem
import com.xplpc.message.Param
import com.xplpc.message.Request
import java.util.concurrent.atomic.AtomicBoolean
import java.util.concurrent.atomic.AtomicInteger
import java.util.concurrent.locks.ReentrantReadWriteLock
import kotlin.concurrent.read
import kotlin.concurrent.write
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertNull

// Which layer serves a name, which is whichever declared it last until it retracts it.
class RoutingTest : PendingGuard() {
    private fun setUp() {
        TestPlatform.initialize()
    }

    // The last layer to declare a name owns it, so a kotlin mapping takes a name c++ registered and clearing it gives the name back.
    @Test
    fun aKotlinMappingTakesACppNameAndClearingGivesItBack() {
        setUp()

        val request =
            Request(
                "sample.login",
                Param("username", "paulo"),
                Param("password", "123456"),
                Param("remember", true)
            )

        try {
            MappingList.add("sample.login", MappingItem { _, r -> r("LOGGED-BY-KOTLIN") })

            assertEquals("LOGGED-BY-KOTLIN", Client.callSync<String>(request))
        } finally {
            MappingList.clear()
        }

        assertEquals("LOGGED-WITH-REMEMBER", Client.callSync<String>(request))
    }

    // Calls racing a name being taken over and given back always reach one of its two owners.
    @Test
    fun aNameTakenOverAndGivenBackUnderLoadAlwaysReachesAnOwner() {
        setUp()

        val request =
            Request(
                "sample.login",
                Param("username", "paulo"),
                Param("password", "123456"),
                Param("remember", true)
            )

        val running = AtomicBoolean(true)
        val unexpected = AtomicInteger()

        // A call already routed to kotlin when the name is retracted finds nothing, which is allowed, so clearing never overlaps a call and only declaring races one.
        val clearing = ReentrantReadWriteLock()

        val caller =
            Thread {
                while (running.get()) {
                    val response = clearing.read { Client.callSync<String>(request) }

                    if (response != "LOGGED-WITH-REMEMBER" && response != "LOGGED-BY-KOTLIN") {
                        unexpected.incrementAndGet()
                    }
                }
            }

        caller.start()

        try {
            repeat(2000) {
                MappingList.add("sample.login", MappingItem { _, r -> r("LOGGED-BY-KOTLIN") })
                clearing.write { MappingList.clear() }
            }
        } finally {
            running.set(false)
            caller.join()
            MappingList.clear()
        }

        assertEquals(0, unexpected.get())
    }

    // A call can reach kotlin for a name it no longer holds a mapping for, since the router routes from what was declared, and the caller is answered rather than left waiting.
    @Test
    fun aCallForANameKotlinDeclaredWithoutAMappingIsAnsweredEmpty() {
        setUp()

        Layer.declare("platform.declared.only")

        try {
            val answers = AtomicInteger()
            var response: String? = "unset"

            Client.call<String>(Request("platform.declared.only")) {
                response = it
                answers.incrementAndGet()
            }

            assertEquals(1, answers.get())
            assertNull(response)
        } finally {
            Layer.retract("platform.declared.only")
        }
    }
}
