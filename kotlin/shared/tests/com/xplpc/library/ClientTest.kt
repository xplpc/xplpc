package com.xplpc.library

import com.google.gson.TypeAdapter
import com.google.gson.annotations.JsonAdapter
import com.google.gson.stream.JsonReader
import com.google.gson.stream.JsonWriter
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
import kotlinx.coroutines.launch
import kotlinx.coroutines.runBlocking
import kotlinx.coroutines.test.runTest
import kotlinx.coroutines.withTimeout
import java.nio.ByteBuffer
import java.util.concurrent.CountDownLatch
import java.util.concurrent.TimeUnit
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFalse
import kotlin.test.assertNotNull
import kotlin.test.assertNull
import kotlin.test.assertTrue

class ClientTest : PendingGuard() {
    @JsonAdapter(UnreadableAdapter::class)
    class Unreadable

    // Gson fills a consumer type through the adapter it names, and an adapter is free to raise an error rather than an exception.
    class UnreadableAdapter : TypeAdapter<Unreadable>() {
        override fun write(
            output: JsonWriter,
            value: Unreadable?
        ) {
            output.nullValue()
        }

        override fun read(input: JsonReader): Unreadable = throw NotImplementedError("the adapter cannot read")
    }

    companion object {
        @Suppress("UNUSED_PARAMETER")
        fun reverseResponse(
            data: Message,
            r: Response
        ) {
            r("ok")
        }

        fun mismatchedType(
            data: Message,
            r: Response
        ) {
            val asString = data.get<String>("value")
            val asInt = data.get<Double>("value")

            r("${asString ?: "nil"}/${asInt?.toInt() ?: -1}")
        }

        fun batteryLevel(
            data: Message,
            r: Response
        ) {
            val level = 100
            val suffix = data.get<String>("suffix")

            if (suffix != null) {
                r("$level$suffix")
            } else {
                r("$level")
            }
        }
    }

    private fun setUp() {
        TestPlatform.initialize()
    }

    // Answers from a thread of its own once the test releases it, and says when it did so a test can wait for it.
    private fun heldAnswer(
        release: CountDownLatch,
        answered: CountDownLatch
    ) = MappingItem { _, r ->
        Thread {
            release.await()
            r("deferred")
            answered.countDown()
        }.start()
    }

    @Test
    fun callAsyncGivesTheSchedulerATurnBeforeAnswering() =
        runBlocking {
            setUp()

            // A call site has to behave the same whether the mapping is quick or slow, so the scheduler is reached even when the answer is already there.

            val order = mutableListOf<String>()

            launch { order.add("scheduler") }

            Client.callAsync<String>(Request("sample.version"))
            order.add("after await")

            assertEquals(listOf("scheduler", "after await"), order)
        }

    @Test
    fun batteryLevel() {
        setUp()

        MappingList.add(
            "platform.battery.level",
            MappingItem(
                ClientTest::batteryLevel
            )
        )

        val request = Request("platform.battery.level", Param("suffix", "%"))

        verifyCall<String>(request) { response ->
            assertEquals("100%", response)
        }
    }

    @Test
    fun batteryLevelCallAsync() =
        runTest {
            setUp()

            MappingList.add(
                "platform.battery.level",
                MappingItem(
                    ClientTest::batteryLevel
                )
            )

            val request = Request("platform.battery.level", Param("suffix", "%"))
            val response: String? = Client.callAsync<String>(request)
            assertEquals("100%", response)
        }

    @Test
    fun batteryLevelInvalidCast() {
        setUp()

        MappingList.add(
            "platform.battery.level",
            MappingItem(
                ClientTest::batteryLevel
            )
        )

        val request = Request("platform.battery.level", Param("suffix", "%"))

        // A value that is not the declared type is answered with nothing, rather than coerced into one that reads as an answer.
        verifyCall<Boolean>(request) { response ->
            assertNull(response)
        }
    }

    @Test
    fun login() {
        setUp()

        val request =
            Request(
                "sample.login",
                Param("username", "paulo"),
                Param("password", "123456"),
                Param("remember", true)
            )

        verifyCall<String>(request) { response ->
            assertEquals("LOGGED-WITH-REMEMBER", response)
        }
    }

    @Test
    fun loginAsync() {
        setUp()

        val request =
            Request(
                "sample.login",
                Param("username", "paulo"),
                Param("password", "123456"),
                Param("remember", true)
            )

        runBlocking {
            assertEquals("LOGGED-WITH-REMEMBER", Client.callAsync<String>(request))
        }
    }

    @Test
    fun loginInvalidCast() {
        setUp()

        val request =
            Request(
                "sample.login",
                Param("username", "paulo"),
                Param("password", "123456"),
                Param("remember", true)
            )

        // A value that is not the declared type is answered with nothing, rather than coerced into one that reads as an answer.
        verifyCall<Boolean>(request) { response ->
            assertNull(response)
        }
    }

    @Test
    fun reverse() {
        setUp()

        MappingList.add(
            "platform.reverse.response",
            MappingItem(
                ClientTest::reverseResponse
            )
        )

        val request = Request("sample.reverse")

        verifyCall<String>(request) { response ->
            assertEquals("response-is-ok", response)
        }
    }

    @Test
    fun aHostMappingThatRaisesAnErrorStillAnswersTheCaller() {
        setUp()

        // An error is not an exception, and a mapping raising one still has to answer the caller rather than leave it waiting.

        MappingList.add(
            "platform.reverse.response",
            MappingItem { _, _ -> throw AssertionError("the host mapping failed") },
        )

        verifyCall<String>(Request("sample.reverse")) { response ->
            assertEquals("response-is-empty", response)
        }
    }

    @Test
    fun reverseAsync() {
        setUp()

        MappingList.add(
            "platform.reverse.response",
            MappingItem(
                ClientTest::reverseResponse
            )
        )

        val request = Request("sample.reverse")

        runBlocking {
            assertEquals("response-is-ok", Client.callAsync<String>(request))
        }
    }

    // The bridge converts utf16 to utf8 by hand, so a string has to survive every width it can be encoded in.
    @Test
    fun textSurvivesEveryEncodingWidth() {
        setUp()

        val samples =
            listOf(
                "plain ascii",
                "caf\u00e9 na\u00efve",
                "\u4e2d\u6587\u30c6\u30b9\u30c8",
                "\ud83d\ude00\ud83d\udc68\u200d\ud83d\udc69\u200d\ud83d\udc67",
                "mixed \u00e9 \u4e2d \ud83d\ude00 end"
            )

        for (sample in samples) {
            verifyCall<String>(Request("sample.echo", Param("value", sample))) { response ->
                assertEquals(sample, response)
            }
        }
    }

    @Test
    fun coreMappingAnswersFromItsOwnThread() {
        setUp()

        val request = Request("sample.async")
        val answered = CountDownLatch(1)

        Client.call<String>(request) {
            answered.countDown()
        }

        assertTrue(answered.await(5, TimeUnit.SECONDS))
    }

    @Test
    fun grayscaleImageWithDataView() {
        setUp()

        // In kotlin the 255 byte value is -1.
        val data =
            byteArrayOf(
                -1, 0, 0, -1,
                0, -1, 0, -1,
                0, 0, -1, -1,
                0, 0, 0, 0,
            )

        val buffer: ByteBuffer = ByteBuffer.allocateDirect(data.size)
        buffer.put(data)

        val dataView = DataView.createFromByteBuffer(buffer)
        val request = Request("sample.image.grayscale.dataview", Param("dataView", dataView))

        // The mapping wrote into the buffer this side registered, so the answer is read back from that buffer itself.
        try {
            verifyCall<String>(request) { response ->
                assertEquals("OK", response)

                assertEquals(16, buffer.capacity())
                assertEquals(buffer[0].toInt(), 85)
                assertEquals(buffer[4].toInt(), 85)
                assertEquals(buffer[8].toInt(), 85)
                assertEquals(buffer[12].toInt(), 0)
            }
        } finally {
            assertTrue(dataView.release())
        }
    }

    @Test
    fun grayscaleImageWithDataViewAsync() {
        setUp()

        // In kotlin the 255 byte value is -1.
        val data =
            byteArrayOf(
                -1, 0, 0, -1,
                0, -1, 0, -1,
                0, 0, -1, -1,
                0, 0, 0, 0,
            )

        val buffer: ByteBuffer = ByteBuffer.allocateDirect(data.size)
        buffer.put(data)

        val dataView = DataView.createFromByteBuffer(buffer)
        val request = Request("sample.image.grayscale.dataview", Param("dataView", dataView))

        try {
            runBlocking {
                val response = Client.callAsync<String>(request)

                assertEquals("OK", response)
                assertEquals(buffer[0].toInt(), 85)
                assertEquals(buffer[4].toInt(), 85)
                assertEquals(buffer[8].toInt(), 85)
                assertEquals(buffer[12].toInt(), 0)
            }
        } finally {
            assertTrue(dataView.release())
        }
    }

    @Test
    fun dataView() {
        setUp()

        val request = Request("sample.dataview")

        verifyCall<DataView>(request) { response ->
            assertNotNull(response)

            val originalData = ByteArrayHelper.createFromDataView(response)

            assertEquals(16, originalData.size)
            assertEquals(originalData[0].toUByte().toInt(), 255)
            assertEquals(originalData[5].toUByte().toInt(), 255)
            assertEquals(originalData[10].toUByte().toInt(), 255)
            assertEquals(originalData[12].toUByte().toInt(), 0)

            val dataView2 = DataView.createFromByteArray(originalData)
            val request2 = Request("sample.image.grayscale.dataview", Param("dataView", dataView2))

            try {
                verifyCall<String>(request2) { response2 ->
                    assertEquals("OK", response2)

                    val processedData = ByteArrayHelper.createFromDataView(dataView2)

                    assertEquals(16, processedData.size)
                    assertEquals(processedData[0].toUByte().toInt(), 85)
                    assertEquals(processedData[5].toUByte().toInt(), 85)
                    assertEquals(processedData[10].toUByte().toInt(), 85)
                    assertEquals(processedData[12].toUByte().toInt(), 0)

                    // The view copied the array, so the source it was built from is untouched.
                    assertEquals(16, originalData.size)
                    assertEquals(originalData[0].toUByte().toInt(), 255)
                    assertEquals(originalData[5].toUByte().toInt(), 255)
                    assertEquals(originalData[10].toUByte().toInt(), 255)
                    assertEquals(originalData[12].toUByte().toInt(), 0)
                }
            } finally {
                assertTrue(dataView2.release())
            }
        }
    }

    @Test
    fun dataViewAsync() {
        setUp()

        runBlocking {
            val response = Client.callAsync<DataView>(Request("sample.dataview"))

            assertNotNull(response)

            val originalData = ByteArrayHelper.createFromDataView(response)

            assertEquals(16, originalData.size)
            assertEquals(originalData[0].toUByte().toInt(), 255)
            assertEquals(originalData[5].toUByte().toInt(), 255)
            assertEquals(originalData[10].toUByte().toInt(), 255)
            assertEquals(originalData[12].toUByte().toInt(), 0)

            val dataView2 = DataView.createFromByteArray(originalData)

            try {
                val response2 = Client.callAsync<String>(Request("sample.image.grayscale.dataview", Param("dataView", dataView2)))

                assertEquals("OK", response2)

                val processedData = ByteArrayHelper.createFromDataView(dataView2)

                assertEquals(16, processedData.size)
                assertEquals(processedData[0].toUByte().toInt(), 85)
                assertEquals(processedData[5].toUByte().toInt(), 85)
                assertEquals(processedData[10].toUByte().toInt(), 85)
                assertEquals(processedData[12].toUByte().toInt(), 0)
            } finally {
                assertTrue(dataView2.release())
            }
        }
    }

    // The c++ side registered the buffer behind this view, so kotlin can read it and cannot release it, whether through the view or straight through the layer.
    @Test
    fun aViewAnotherLayerRegisteredIsNotReleasedByKotlin() {
        setUp()

        val response = assertNotNull(Client.callSync<DataView>(Request("sample.dataview")))

        assertFalse(response.release())
        assertFalse(Layer.releaseBuffer(response.buffer))
        assertEquals(16, ByteArrayHelper.createFromDataView(response).size)
    }

    @Test
    fun batteryLevelFromString() {
        setUp()

        MappingList.add(
            "platform.battery.level",
            MappingItem(
                ClientTest::batteryLevel
            )
        )

        val request = Request("platform.battery.level", Param("suffix", "%"))

        verifyCall(request.data) { response ->
            assertEquals("{\"r\":\"100%\"}", response)
        }
    }

    @Test
    fun batteryLevelCallAsyncFromString() =
        runTest {
            setUp()

            MappingList.add("platform.battery.level", MappingItem(ClientTest::batteryLevel))

            val request = Request("platform.battery.level", Param("suffix", "%"))
            val response = Client.callAsync(request.data)
            assertEquals("{\"r\":\"100%\"}", response)
        }

    // A buffer far past what a small integer holds has to cross without being truncated anywhere on the way.
    @Test
    fun largeDataViewCrossesWithoutTruncating() {
        setUp()

        val size = 4 * 1024 * 1024
        val buffer: ByteBuffer = ByteBuffer.allocateDirect(size)

        for (i in 0 until size) {
            buffer.put(i, 100.toByte())
        }

        val dataView = DataView.createFromByteBuffer(buffer)
        val request = Request("sample.image.grayscale.dataview", Param("dataView", dataView))

        try {
            verifyCall<String>(request) { response ->
                assertEquals("OK", response)
                assertEquals(size, ByteArrayHelper.createFromDataView(dataView).size)
                assertEquals(100, buffer[0].toInt())
                assertEquals(100, buffer[size - 1].toInt())
            }
        } finally {
            assertTrue(dataView.release())
        }
    }

    @Test
    fun nestedHostMappingAnsweringLaterStillReachesTheOuterCaller() {
        setUp()

        val held = mutableListOf<Response>()
        MappingList.add("platform.reverse.response", MappingItem { _, r -> held.add(r) })

        val answered = CountDownLatch(1)
        var response: String? = null

        Client.call<String>(Request("sample.reverse")) {
            response = it
            answered.countDown()
        }

        assertEquals(1, held.size)
        assertEquals(1L, answered.count, "the outer call answered before the inner one did")

        held.single()("ok")

        assertTrue(answered.await(5, TimeUnit.SECONDS))
        assertEquals("response-is-ok", response)
    }

    @Test
    fun hostMappingAnswersAfterTheCallReturned() {
        setUp()

        val release = CountDownLatch(1)
        val mapped = CountDownLatch(1)

        MappingList.add("platform.deferred.answer", heldAnswer(release, mapped))

        val answered = CountDownLatch(1)
        var response: String? = null

        Client.call<String>(Request("platform.deferred.answer")) {
            response = it
            answered.countDown()
        }

        val returnedBeforeAnswer = answered.count > 0

        release.countDown()

        assertTrue(answered.await(5, TimeUnit.SECONDS))
        assertEquals("deferred", response)
        assertTrue(returnedBeforeAnswer)
    }

    @Test
    fun mappingReadingAMismatchedTypeGetsNothing() {
        setUp()

        MappingList.add(
            "platform.mismatched.type",
            MappingItem(
                ClientTest::mismatchedType
            )
        )

        val request = Request("platform.mismatched.type", Param("value", 10))

        verifyCall<String>(request) { response ->
            assertEquals("nil/10", response)
        }
    }

    @Test
    fun unknownFunctionAnswersEmpty() {
        setUp()

        verifyCall<String>(Request("not.found")) { response ->
            assertEquals(null, response)
        }
    }

    @Test
    fun callSyncAnswersTheValueFromAnInlineMapping() {
        setUp()

        MappingList.add(
            "platform.battery.level",
            MappingItem(
                ClientTest::batteryLevel
            )
        )

        val request = Request("platform.battery.level", Param("suffix", "%"))

        assertEquals("100%", Client.callSync<String>(request))
    }

    @Test
    fun callSyncAnswersTheValueFromAnInlineNativeMapping() {
        setUp()

        val request =
            Request(
                "sample.login",
                Param("username", "paulo"),
                Param("password", "123456"),
                Param("remember", true)
            )

        assertEquals("LOGGED-WITH-REMEMBER", Client.callSync<String>(request))
    }

    @Test
    fun callSyncAnswersEmptyWhenTheMappingDefers() {
        // A mapping that answers later cannot be read synchronously, and the call it would have resolved is cancelled rather than left behind.

        setUp()

        val release = CountDownLatch(1)
        val answered = CountDownLatch(1)

        MappingList.add("platform.deferred.answer", heldAnswer(release, answered))

        assertEquals(null, Client.callSync<String>(Request("platform.deferred.answer")))
        assertEquals(0L, Layer.pending())

        // The mapping answers a call that is already gone, and the suite waits for it rather than leaving it running past the test.
        release.countDown()
        assertTrue(answered.await(5, TimeUnit.SECONDS))
    }

    @Test
    fun callSyncAnswersEmptyWhenNothingOwnsTheFunction() {
        setUp()

        assertEquals(null, Client.callSync<String>(Request("not.found")))
    }

    @Test
    fun callSyncAnswersTheDocumentFromAString() {
        setUp()

        val request =
            "{\"f\":\"sample.login\",\"p\":[{\"n\":\"username\",\"v\":\"paulo\"}," +
                "{\"n\":\"password\",\"v\":\"123456\"},{\"n\":\"remember\",\"v\":true}]}"

        assertEquals("{\"r\":\"LOGGED-WITH-REMEMBER\"}", Client.callSync(request))
    }

    @Test
    fun callSyncAnswersEmptyFromAStringWhenTheMappingDefers() {
        setUp()

        val release = CountDownLatch(1)
        val answered = CountDownLatch(1)

        MappingList.add("platform.deferred.answer", heldAnswer(release, answered))

        assertEquals("", Client.callSync("{\"f\":\"platform.deferred.answer\",\"p\":[]}"))
        assertEquals(0L, Layer.pending())

        release.countDown()
        assertTrue(answered.await(5, TimeUnit.SECONDS))
    }

    @Test
    fun callSyncAnswersEmptyFromAStringWhenNothingOwnsTheFunction() {
        setUp()

        assertEquals("", Client.callSync("{\"f\":\"not.found\",\"p\":[]}"))
    }

    @Test
    fun callSyncAnswersWhatTheNestedMappingProduced() {
        setUp()

        MappingList.add(
            "platform.reverse.response",
            MappingItem(
                ClientTest::reverseResponse
            )
        )

        assertEquals("response-is-ok", Client.callSync<String>(Request("sample.reverse")))
    }

    @Test
    fun textThatIsNotWellFormedCrossesAsTheReplacementCharacter() {
        // A string may hold a surrogate with no pair, which utf8 cannot represent, so the bridge carries what it can rather than writing bytes no reader accepts.

        setUp()

        val samples =
            mapOf(
                "\ud800" to "\ufffd",
                "a\udc00b" to "a\ufffdb",
                "ok\ud83d" to "ok\ufffd"
            )

        for ((sample, expected) in samples) {
            verifyCall<String>(Request("sample.echo", Param("value", sample))) { response ->
                assertEquals(expected, response)
            }
        }
    }

    @Test
    fun aRequestWhoseValueOverflowsTheEncoderIsAnsweredEmpty() {
        setUp()

        // Gson writes a list that holds itself until the stack runs out, which is an error rather than an exception.
        val itself = mutableListOf<Any>()
        itself.add(itself)

        val request = Request("sample.echo", Param("value", itself))

        assertEquals("", request.data)

        verifyCall<String>(request) { response ->
            assertNull(response)
        }
    }

    @Test
    fun aMappingAnsweringWhatCannotBeEncodedFromItsOwnThreadIsAnsweredEmpty() {
        // Gson writes a list that holds itself until the stack runs out, and on the mapping's own thread nothing above the responder would answer the caller.

        setUp()

        MappingList.add(
            "platform.unencodable.later",
            MappingItem { _, r ->
                Thread {
                    val itself = mutableListOf<Any>()
                    itself.add(itself)
                    r(itself)
                }.start()
            }
        )

        val answered = CountDownLatch(1)
        var response: String? = "unanswered"

        Client.call<String>(Request("platform.unencodable.later")) {
            response = it
            answered.countDown()
        }

        assertTrue(answered.await(5, TimeUnit.SECONDS))
        assertNull(response)
    }

    @Test
    fun anAnswerWhoseTypeRaisesAnErrorWhileDecodedStillAnswersTheCaller() {
        setUp()

        val request = Request("sample.echo", Param("value", "unreadable"))

        verifyCall<Unreadable>(request) { response ->
            assertNull(response)
        }

        runBlocking {
            assertNull(withTimeout(5000) { Client.callAsync<Unreadable>(request) })
        }
    }
}
