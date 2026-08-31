package com.xplpc.library

import com.xplpc.data.CallbackList
import com.xplpc.data.MappingList
import com.xplpc.helper.ByteArrayHelper
import com.xplpc.map.MappingItem
import com.xplpc.message.Message
import com.xplpc.type.DataView
import java.nio.ByteBuffer
import java.util.concurrent.atomic.AtomicInteger
import kotlin.test.AfterTest
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFalse
import kotlin.test.assertNull

class DataListTest {
    private fun setUp() {
        TestPlatform.initialize()
    }

    @AfterTest
    fun tearDown() {
        CallbackList.clear()
    }

    @Test
    fun callbackListExecutesOnlyOnce() {
        setUp()

        var calls = 0

        CallbackList.add("callback-once") { calls += 1 }

        CallbackList.execute("callback-once", "")
        CallbackList.execute("callback-once", "")

        assertEquals(1, calls)
    }

    @Test
    fun callbackListRemovesPendingEntry() {
        setUp()

        var calls = 0

        CallbackList.add("callback-removed") { calls += 1 }
        CallbackList.remove("callback-removed")
        CallbackList.execute("callback-removed", "")

        assertEquals(0, calls)
        assertEquals(0, CallbackList.count())
    }

    @Test
    fun callbackListIsTakenByASingleThread() {
        setUp()

        val calls = AtomicInteger()

        CallbackList.add("callback-concurrent") { calls.incrementAndGet() }

        val threads = (1..16).map {
            Thread { CallbackList.execute("callback-concurrent", "") }
        }

        threads.forEach { it.start() }
        threads.forEach { it.join() }

        assertEquals(1, calls.get())
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

        // The source array is dropped and the heap is churned, and the view must still address its own storage.
        System.gc()
        System.gc()

        repeat(64) { ByteArray(64 * 1024) }

        assertEquals(8, dataView.size)
        assertEquals(data.toList(), ByteArrayHelper.createFromDataView(dataView).toList())
    }

    @Test
    fun messageValueReturnsNullForAnotherType() {
        val message = Message()
        message.set("number", 10)

        assertEquals(10, message.get<Int>("number"))
        assertNull(message.get<String>("number"))
        assertNull(message.get<Int>("missing"))
    }

    // A view that carries no address describes nothing to read, and reading it must not fault.
    @Test
    fun emptyDataViewReadsAsNothing() {
        assertEquals(0, ByteArrayHelper.createFromDataView(DataView(0, 16)).size)
        assertEquals(0, ByteArrayHelper.createFromDataView(DataView(128, 0)).size)
        assertEquals(0, ByteArrayHelper.createFromDataView(DataView(128, -1)).size)
    }

    // A heap buffer has no address native code can use, so the view has to refuse it rather than point at zero.
    @Test
    fun heapBufferIsRefused() {
        val view = DataView.createFromByteBuffer(ByteBuffer.allocate(16))

        assertEquals(0L, view.ptr)
        assertEquals(0, view.size)
    }
}
