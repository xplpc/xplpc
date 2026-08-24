package com.xplpc.library

import com.xplpc.core.Config
import com.xplpc.core.XPLPC
import com.xplpc.serializer.JsonSerializer
import kotlin.test.Test
import kotlin.test.assertSame

class CoreTest {
    private fun setUp() {
        TestPlatform.initialize()
    }

    @Test
    fun initialize() {
        setUp()
        assert(XPLPC.initialized)
    }

    @Test
    fun initializeAgainKeepsTheFirstConfig() {
        setUp()

        val serializer = XPLPC.config.serializer

        XPLPC.initialize(Config(JsonSerializer()))

        assertSame(serializer, XPLPC.config.serializer)
    }
}
