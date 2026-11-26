package com.xplpc.library

import com.google.gson.JsonParser
import com.google.gson.reflect.TypeToken
import com.xplpc.data.MappingList
import com.xplpc.helper.ByteArrayHelper
import com.xplpc.layer.Layer
import com.xplpc.map.MappingItem
import com.xplpc.message.Message
import com.xplpc.serializer.JsonSerializer
import com.xplpc.type.DataView
import java.lang.ref.WeakReference
import java.nio.ByteBuffer
import java.util.concurrent.TimeUnit
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFalse
import kotlin.test.assertNotNull
import kotlin.test.assertNull
import kotlin.test.assertTrue

class DataListTest {
    private fun setUp() {
        TestPlatform.initialize()
    }

    @Test
    fun mappingListSurvivesClear() {
        setUp()

        MappingList.add("sample.temporary", MappingItem { _, r -> r(null) })
        MappingList.clear()

        assertEquals(0, MappingList.count())
        assertFalse(MappingList.has("sample.temporary"))
    }

    @Test
    fun dataViewFromByteArraySurvivesGarbageCollection() {
        setUp()

        val data = byteArrayOf(1, 2, 3, 4, 5, 6, 7, 8)
        val dataView = DataView.createFromByteArray(data)

        // The source array is dropped and the heap is churned, and the view must still name its own copy.
        System.gc()
        System.gc()

        repeat(64) { ByteArray(64 * 1024) }

        try {
            assertEquals(data.toList(), ByteArrayHelper.createFromDataView(dataView).toList())
        } finally {
            assertTrue(dataView.release())
        }
    }

    @Test
    fun messageValueReturnsNullForAnotherType() {
        setUp()

        val message = Message()
        message.set("number", 10)

        assertEquals(10, message.get<Int>("number"))
        assertNull(message.get<String>("number"))
        assertNull(message.get<Int>("missing"))
    }

    // Only the handle and a weak reference leave this frame, so whatever keeps the buffer alive afterwards is the registration.
    private fun registerABufferNothingElseHolds(): Pair<Long, WeakReference<ByteBuffer>> {
        val buffer = ByteBuffer.allocateDirect(16)
        buffer.put(0, 7)

        return DataView.createFromByteBuffer(buffer).buffer to WeakReference(buffer)
    }

    // An android runtime before api 30 treats System.gc as a hint it acts on only after finalization ran, so the collection is asked for directly.
    private fun collectGarbage() {
        repeat(8) {
            Runtime.getRuntime().gc()
            System.runFinalization()
            Thread.sleep(10)
        }
    }

    // The native side holds the buffer for as long as it is registered, so neither the view nor the caller has to, and releasing it is what lets it go.
    @Test
    fun aRegisteredBufferLivesUntilItIsReleased() {
        setUp()

        val (handle, buffer) = registerABufferNothingElseHolds()

        collectGarbage()

        assertNotNull(buffer.get(), "the buffer was collected while it was still registered")
        assertEquals(7, ByteArrayHelper.createFromDataView(viewOf(handle))[0].toInt())
        assertTrue(Layer.releaseBuffer(handle))

        val deadline = System.nanoTime() + TimeUnit.SECONDS.toNanos(5)

        while (buffer.get() != null && System.nanoTime() < deadline) {
            collectGarbage()
        }

        assertNull(buffer.get(), "the buffer outlived its registration")
    }

    // Releasing is what makes every reader of the view read nothing, and a second release has nothing left to let go.
    @Test
    fun aReleasedViewReadsNothing() {
        setUp()

        val view = DataView.createFromByteArray(byteArrayOf(1, 2, 3, 4))

        assertEquals(4, ByteArrayHelper.createFromDataView(view).size)
        assertTrue(view.release())

        assertEquals(0, ByteArrayHelper.createFromDataView(view).size)
        assertNull(Layer.readBuffer(view.buffer))
        assertFalse(view.release())
    }

    // A view read off the wire names a buffer some other view registered, so it can read the bytes and cannot let them go.
    @Test
    fun aViewIsReleasedOnlyByTheViewThatRegisteredIt() {
        setUp()

        val owner = DataView.createFromByteArray(byteArrayOf(9, 8, 7, 6))
        val named =
            assertNotNull(
                JsonSerializer()
                    .decodeValue(JsonParser.parseString("{\"buffer\":${owner.buffer}}"), object : TypeToken<DataView>() {})
            )

        try {
            assertEquals(owner.buffer, named.buffer)
            assertFalse(named.release())
            assertEquals(listOf<Byte>(9, 8, 7, 6), ByteArrayHelper.createFromDataView(named).toList())
        } finally {
            assertTrue(owner.release())
        }

        assertEquals(0, ByteArrayHelper.createFromDataView(named).size)
    }

    // A view that names no registered buffer describes nothing to read, and reading it must not fault.
    @Test
    fun aViewNamingNoBufferReadsAsNothing() {
        setUp()

        assertEquals(0, ByteArrayHelper.createFromDataView(viewOf(0)).size)
        assertEquals(0, ByteArrayHelper.createFromDataView(viewOf((0x7FFF_FFFEL shl 32) or 1L)).size)
        assertEquals(0, ByteArrayHelper.createFromDataView(viewOf(-1)).size)
        assertFalse(viewOf(0).release())
    }

    // A heap buffer has no address native code can hold, so the view has to refuse it rather than register it.
    @Test
    fun heapBufferIsRefused() {
        setUp()

        val view = DataView.createFromByteBuffer(ByteBuffer.allocate(16))

        assertEquals(0L, view.buffer)
        assertEquals(0, ByteArrayHelper.createFromDataView(view).size)
        assertFalse(view.release())
    }

    private fun viewOf(handle: Long): DataView =
        assertNotNull(
            JsonSerializer()
                .decodeValue(JsonParser.parseString("{\"buffer\":$handle}"), object : TypeToken<DataView>() {})
        )
}
