package com.xplpc.library

import com.xplpc.client.Client
import com.xplpc.data.MappingList
import com.xplpc.data.PendingCall
import com.xplpc.layer.Layer
import com.xplpc.map.MappingItem
import com.xplpc.message.Message
import com.xplpc.message.Param
import com.xplpc.message.Request
import com.xplpc.message.Response
import java.util.Collections
import java.util.concurrent.ConcurrentHashMap
import java.util.concurrent.CountDownLatch
import java.util.concurrent.atomic.AtomicInteger
import kotlin.test.AfterTest
import kotlin.test.BeforeTest
import kotlin.test.Test
import kotlin.test.assertEquals

class StressTest : PendingGuard() {
    companion object {
        private const val THREAD_COUNT = 16
        private const val ITERATION_COUNT = 400

        fun echo(
            data: Message,
            r: Response
        ) {
            r(data.get<String>("value") ?: "")
        }
    }

    private fun setUp() {
        TestPlatform.initialize()
    }

    // Each test starts from a clean registry, so the order they run in cannot change the result.
    @BeforeTest
    fun prepare() {
        MappingList.clear()
    }

    @AfterTest
    fun tearDown() {
        MappingList.clear()
    }

    private fun runConcurrently(block: (Int) -> Unit) {
        // Every thread starts at the same instant, so the router sees the worst contention.

        val start = CountDownLatch(1)
        val done = CountDownLatch(THREAD_COUNT)

        val threads =
            (0 until THREAD_COUNT).map { index ->
                Thread {
                    start.await()

                    try {
                        block(index)
                    } finally {
                        done.countDown()
                    }
                }
            }

        threads.forEach { it.start() }
        start.countDown()
        done.await()
    }

    @Test
    fun mappingListSurvivesContention() {
        setUp()

        runConcurrently { index ->
            repeat(ITERATION_COUNT) { i ->
                val name = "stress.mapping.${i % 16}"

                MappingList.add(name, MappingItem(::echo))
                MappingList.find(name)
                MappingList.has(name)
                MappingList.count()

                if (index == 0 && i % 64 == 0) {
                    MappingList.clear()
                }
            }
        }
    }

    @Test
    fun clientAnswersEveryCallFromEveryThread() {
        setUp()

        MappingList.add("stress.echo", MappingItem(::echo))

        val answered = AtomicInteger()
        val wrong = AtomicInteger()

        runConcurrently {
            repeat(64) {
                val request = Request("stress.echo", Param("value", "stress"))

                Client.call<String>(request) { response ->
                    if (response == "stress") {
                        answered.incrementAndGet()
                    } else {
                        wrong.incrementAndGet()
                    }
                }
            }
        }

        assertEquals(THREAD_COUNT * 64, answered.get())
        assertEquals(0, wrong.get())
    }

    // Half the calls are cancelled while the mapping holds them, and every one of them is answered afterwards, so a handle that resolves the wrong call or resolves twice shows up as a count.
    @Test
    fun cancelledAndAnsweredCallsEachResolveAtMostOnce() {
        setUp()

        val held = Collections.synchronizedList(mutableListOf<Response>())

        MappingList.add("stress.held", MappingItem { _, r -> held.add(r) })

        val answers = ConcurrentHashMap<Int, AtomicInteger>()
        val cancelled = Collections.newSetFromMap(ConcurrentHashMap<Int, Boolean>())
        val refused = AtomicInteger()

        runConcurrently { thread ->
            repeat(64) { iteration ->
                val id = thread * 64 + iteration
                val count = AtomicInteger()

                answers[id] = count

                val call = Layer.call(Request("stress.held").data, PendingCall { count.incrementAndGet() })

                if (iteration % 2 != 0) {
                    return@repeat
                }

                cancelled.add(id)

                if (!Layer.cancel(call)) {
                    refused.incrementAndGet()
                }
            }
        }

        assertEquals(0, refused.get())
        assertEquals(THREAD_COUNT * 64, held.size)

        runConcurrently { thread ->
            for (index in thread until held.size step THREAD_COUNT) {
                held[index]("late")
            }
        }

        for ((id, count) in answers) {
            val expected = if (id in cancelled) 0 else 1
            assertEquals(expected, count.get(), "call $id")
        }
    }
}
