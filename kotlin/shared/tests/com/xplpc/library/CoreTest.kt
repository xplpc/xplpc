package com.xplpc.library

import com.xplpc.core.Config
import com.xplpc.core.XPLPC
import com.xplpc.serializer.JsonSerializer
import kotlin.test.Test
import kotlin.test.assertSame
import kotlin.test.assertTrue

class CoreTest {
    private fun setUp() {
        TestPlatform.initialize()
    }

    @Test
    fun initialize() {
        setUp()
        assertTrue(XPLPC.initialized)
    }

    @Test
    fun initializeAgainKeepsTheFirstConfig() {
        setUp()

        val serializer = XPLPC.config.serializer

        XPLPC.initialize(Config(JsonSerializer()))

        assertSame(serializer, XPLPC.config.serializer)
    }
}
