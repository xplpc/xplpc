package com.xplpc.library

import com.xplpc.client.Client
import com.xplpc.data.CallbackList
import com.xplpc.util.UniqueID
import com.xplpc.data.MappingList
import com.xplpc.map.MappingItem
import com.xplpc.message.Message
import com.xplpc.message.Param
import com.xplpc.message.Request
import com.xplpc.message.Response
import java.util.concurrent.CountDownLatch
import java.util.concurrent.atomic.AtomicInteger
import kotlin.test.AfterTest
import kotlin.test.BeforeTest
import kotlin.test.Test
import kotlin.test.assertEquals

class StressTest {
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
        CallbackList.clear()
        MappingList.clear()
    }

    @AfterTest
    fun tearDown() {
        CallbackList.clear()
        MappingList.clear()
    }

    private fun runConcurrently(block: (Int) -> Unit) {
        // Every thread starts at the same instant, so the registries see the worst contention.

        val start = CountDownLatch(1)
        val done = CountDownLatch(THREAD_COUNT)

        val threads = (0 until THREAD_COUNT).map { index ->
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
    fun uniqueIDIsDistinctUnderContention() {
        setUp()

        val keys = java.util.Collections.synchronizedList(mutableListOf<String>())

        runConcurrently {
            repeat(ITERATION_COUNT) {
                keys.add(UniqueID.generate())
            }
        }

        assertEquals(THREAD_COUNT * ITERATION_COUNT, keys.size)
        assertEquals(keys.size, keys.toSet().size)
    }

    @Test
    fun callbackListSurvivesContention() {
        setUp()

        val executed = AtomicInteger()

        runConcurrently { index ->
            repeat(ITERATION_COUNT) { i ->
                val key = "stress-${i % 32}"

                CallbackList.add(key) { executed.incrementAndGet() }
                CallbackList.execute(key, "data")
                CallbackList.count()

                if (index % 2 == 0) {
                    CallbackList.remove(key)
                }
            }
        }

        assert(executed.get() > 0)
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

        runConcurrently {
            repeat(64) {
                val request = Request("stress.echo", Param("value", "stress"))

                Client.call<String>(request) { response ->
                    if (response == "stress") {
                        answered.incrementAndGet()
                    }
                }
            }
        }

        assertEquals(THREAD_COUNT * 64, answered.get())
        assertEquals(0, CallbackList.count())
    }
}
