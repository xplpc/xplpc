package com.xplpc.library

import com.xplpc.client.Client
import com.xplpc.data.MappingList
import com.xplpc.helper.ByteArrayHelper
import com.xplpc.layer.Layer
import com.xplpc.map.MappingItem
import com.xplpc.message.Message
import com.xplpc.message.Param
import com.xplpc.message.Request
import com.xplpc.message.Response
import com.xplpc.type.DataView
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.cancelAndJoin
import kotlinx.coroutines.joinAll
import kotlinx.coroutines.launch
import kotlinx.coroutines.runBlocking
import java.util.concurrent.ConcurrentLinkedQueue
import java.util.concurrent.CountDownLatch
import java.util.concurrent.TimeUnit
import java.util.concurrent.atomic.AtomicReference
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertNotNull
import kotlin.test.assertNull
import kotlin.test.assertTrue

// What a call keeps alive and what it lets go, which is where the buffers behind a view and the handles of a cancelled call are won or lost.
class CallLifetimeTest : PendingGuard() {
    private fun setUp() {
        TestPlatform.initialize()
    }

    // An android runtime before api 30 treats System.gc as a hint it acts on only after finalization ran, so the collection is asked for directly.
    private fun collectGarbage() {
        repeat(8) {
            Runtime.getRuntime().gc()
            System.runFinalization()
            Thread.sleep(10)
        }
    }

    // The document carries only a handle, so the registration is what keeps the bytes readable through the call and after it, until the caller releases them.
    @Test
    fun aViewStaysReadableUntilTheCallIsAnsweredAndTheCallerReleasesIt() {
        setUp()

        val bytes = byteArrayOf(1, 2, 3, 4, 5, 6, 7, 8)
        val reached = CountDownLatch(1)
        val answered = CountDownLatch(1)
        val held = AtomicReference<Pair<Message, Response>>()

        MappingList.add(
            "platform.deferred.reader",
            MappingItem { data, r ->
                held.set(data to r)
                reached.countDown()
            }
        )

        // The view copied the bytes into a buffer only the registration holds, so the collection below is what would take them if nothing else did.
        val view = DataView.createFromByteArray(bytes)

        Client.call<String>(Request("platform.deferred.reader", Param("dataView", view))) {
            answered.countDown()
        }

        assertTrue(reached.await(5, TimeUnit.SECONDS))

        val (message, respond) = held.get()

        // The held call is answered whatever the checks find, so a failure here is reported as itself rather than as the call it left behind.
        try {
            collectGarbage()

            val received = message.get<DataView>("dataView")

            assertNotNull(received)
            assertEquals(view.buffer, received.buffer)
            assertEquals(bytes.toList(), ByteArrayHelper.createFromDataView(received).toList())
        } finally {
            respond("done")
        }

        assertTrue(answered.await(5, TimeUnit.SECONDS))
        assertEquals(bytes.toList(), ByteArrayHelper.createFromDataView(view).toList(), "the buffer was let go before the release")
        assertTrue(view.release())
        assertNull(Layer.readBuffer(view.buffer), "the buffer is still registered after the caller released it")
    }

    // Cancelling is what lets the router drop a call the mapping still holds, so nothing is left waiting even before the mapping answers.
    @Test
    fun cancellingCallAsyncDropsTheCallBeforeTheMappingAnswers() =
        runBlocking {
            setUp()

            val held = ConcurrentLinkedQueue<Response>()
            val reached = CountDownLatch(1)

            MappingList.add(
                "platform.held.answer",
                MappingItem { _, r ->
                    held.add(r)
                    reached.countDown()
                }
            )

            val job =
                launch(Dispatchers.Default) {
                    Client.callAsync<String>(Request("platform.held.answer"))
                }

            try {
                assertTrue(reached.await(5, TimeUnit.SECONDS))
                job.cancelAndJoin()

                assertEquals(0L, Layer.pending(), "the cancelled call is still waiting in the router")
            } finally {
                held.forEach { it(null) }
            }
        }

    // A cancellation can land at any point of registering a call, and wherever it lands the router must not keep the call.
    @Test
    fun cancellingCallAsyncWhileItRegistersLeavesNothingPending() =
        runBlocking {
            setUp()

            val held = ConcurrentLinkedQueue<Response>()

            MappingList.add("platform.held.answer", MappingItem { _, r -> held.add(r) })

            val jobs =
                (0 until 300).map { index ->
                    val job =
                        launch(Dispatchers.Default) {
                            if (index % 2 == 0) {
                                Client.callAsync<String>(Request("platform.held.answer"))
                            } else {
                                Client.callAsync(Request("platform.held.answer").data)
                            }
                        }

                    // The cancellations are spread over the time a call takes to register, since one that lands before the call starts never reaches it.
                    val cancelAt = System.nanoTime() + (index % 20) * 100_000L

                    while (System.nanoTime() < cancelAt) {
                        Thread.yield()
                    }

                    job.cancel()

                    job
                }

            jobs.joinAll()

            try {
                assertEquals(0L, Layer.pending(), "a cancelled call is still waiting in the router")
            } finally {
                // The mapping is still holding the calls it received, and answering them is a no-op for every one that was cancelled.
                held.forEach { it(null) }
            }
        }
}
