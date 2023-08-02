package com.xplpc.library

import com.xplpc.core.XPLPC
import com.xplpc.platform.PlatformInitializer
import kotlin.test.Test

class CoreTest {
    private fun setUp() {
        PlatformInitializer.initialize()
    }

    @Test
    fun initialize() {
        setUp()
        assert(XPLPC.initialized)
    }
}
