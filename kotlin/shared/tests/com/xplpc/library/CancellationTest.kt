package com.xplpc.library

import com.xplpc.client.Client
import com.xplpc.data.MappingList
import com.xplpc.data.PendingCall
import com.xplpc.layer.Layer
import com.xplpc.map.MappingItem
import com.xplpc.message.Request
import com.xplpc.message.Response
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.async
import kotlinx.coroutines.cancelAndJoin
import kotlinx.coroutines.job
import kotlinx.coroutines.launch
import kotlinx.coroutines.runBlocking
import kotlinx.coroutines.withTimeoutOrNull
import java.util.concurrent.LinkedBlockingQueue
import java.util.concurrent.TimeUnit
import java.util.concurrent.atomic.AtomicInteger
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFalse
import kotlin.test.assertNotNull
import kotlin.test.assertTrue

// A cancelled call is dropped by the router wherever the cancellation lands, so its caller returns at once and nothing answers it afterwards.
class CancellationTest : PendingGuard() {
    private fun setUp() {
        TestPlatform.initialize()
    }

    // A caller cancelled while the mapping still holds the call takes the call back out of the router, so the answer the mapping gives afterwards resolves nothing.
    @Test
    fun cancellingCallAsyncWhileTheMappingWorksTakesTheCallBack() =
        runBlocking {
            setUp()

            val held = LinkedBlockingQueue<Response>()

            MappingList.add(
                "platform.held.answer",
                MappingItem { _, r -> held.put(r) }
            )

            val job =
                launch(Dispatchers.Default) {
                    Client.callAsync<String>(Request("platform.held.answer"))
                }

            val responder = assertNotNull(held.poll(5, TimeUnit.SECONDS))

            job.cancelAndJoin()

            assertTrue(job.isCancelled)
            assertEquals(0L, Layer.pending())

            responder("late")

            assertEquals(0L, Layer.pending())
        }

    @Test
    fun callAsyncCancelledBeforeItStartsNeverReachesTheMapping() =
        runBlocking {
            setUp()

            val reached = AtomicInteger()

            MappingList.add(
                "platform.counted.answer",
                MappingItem { _, r ->
                    reached.incrementAndGet()
                    r("reached")
                }
            )

            val job =
                launch(Dispatchers.Default) {
                    coroutineContext.job.cancel()
                    Client.callAsync<String>(Request("platform.counted.answer"))
                }

            job.join()

            assertTrue(job.isCancelled)
            assertEquals(0, reached.get())
        }

    // The answer and the cancellation arrive from two threads at once, and whichever wins the caller is resumed at most once and never left waiting.
    @Test
    fun cancellingCallAsyncWhileTheMappingAnswersResolvesOnce() =
        runBlocking {
            setUp()

            val held = LinkedBlockingQueue<Response>()

            MappingList.add("platform.held.answer", MappingItem { _, r -> held.add(r) })

            repeat(200) {
                val call = async(Dispatchers.Default) { Client.callAsync<String>(Request("platform.held.answer")) }
                val respond = assertNotNull(held.poll(5, TimeUnit.SECONDS))
                val answering = Thread { respond("raced") }

                answering.start()
                call.cancel()
                answering.join()

                assertEquals(true, withTimeoutOrNull(5000) { call.join() == Unit }, "the call was left waiting")

                if (!call.isCancelled) {
                    assertEquals("raced", call.await())
                }
            }
        }

    // A cancelled call is dropped by the router, so the answer the mapping gives afterwards resolves nothing.
    @Test
    fun aCancelledDeferredCallNeverInvokesItsCallback() {
        setUp()

        val held = LinkedBlockingQueue<Response>()

        MappingList.add("platform.held.answer", MappingItem { _, r -> held.add(r) })

        val invoked = AtomicInteger()
        val call = Layer.call(Request("platform.held.answer").data, PendingCall { invoked.incrementAndGet() })

        assertTrue(call != 0L)

        val respond = assertNotNull(held.poll(5, TimeUnit.SECONDS))

        assertTrue(Layer.cancel(call))
        assertFalse(Layer.cancel(call))
        assertEquals(0L, Layer.pending())

        val answering = Thread { respond("late") }
        answering.start()
        answering.join()

        assertEquals(0, invoked.get())
    }
}
