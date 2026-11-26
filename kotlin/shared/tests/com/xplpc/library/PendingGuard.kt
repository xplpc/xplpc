package com.xplpc.library

import com.xplpc.layer.Layer
import kotlin.test.AfterTest
import kotlin.test.assertEquals

// A call left waiting in the router is the defect this project has found most often, so every test that makes a call is held to leaving none.
abstract class PendingGuard {
    @AfterTest
    fun leavesNothingPending() {
        // The library is brought up here too, since a test that never needed it would otherwise fail on the native call below.
        TestPlatform.initialize()

        assertEquals(0L, Layer.pending(), "a call is still waiting in the router")
    }
}
