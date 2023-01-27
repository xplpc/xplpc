package com.xplpc.library

import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.test.platform.app.InstrumentationRegistry
import com.xplpc.core.Config
import com.xplpc.core.XPLPC
import com.xplpc.serializer.JsonSerializer
import org.junit.Assert.assertEquals
import org.junit.Test
import org.junit.runner.RunWith

@RunWith(AndroidJUnit4::class)
class CoreTest {
    @Test
    fun useAppContext() {
        val appContext = InstrumentationRegistry.getInstrumentation().targetContext
        assertEquals("com.xplpc.library.test", appContext.packageName)
    }

    @Test
    fun initialize() {
        XPLPC.initialize(
            Config(JsonSerializer()),
        )

        assert(XPLPC.initialized)
    }
}
