package com.xplpc.library

import com.xplpc.client.Client
import kotlinx.coroutines.cancelAndJoin
import kotlinx.coroutines.delay
import kotlinx.coroutines.launch
import com.xplpc.data.CallbackList
import com.xplpc.data.MappingList
import com.xplpc.helper.ByteArrayHelper
import com.xplpc.helper.ByteBufferHelper
import com.xplpc.map.MappingItem
import com.xplpc.message.Message
import com.xplpc.message.Param
import com.xplpc.message.Request
import com.xplpc.message.Response
import com.xplpc.type.DataView
import java.util.concurrent.CountDownLatch
import java.util.concurrent.TimeUnit
import kotlinx.coroutines.runBlocking
import kotlinx.coroutines.test.runTest
import java.nio.ByteBuffer
import kotlin.test.AfterTest
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertNotNull
import kotlin.test.assertTrue

class ClientTest {
    @AfterTest
    fun leavesNothingRegistered() {
        // A leaked callback is the defect this project has found most often, so every test is held to leaving none.
        assertEquals(0, CallbackList.count())
    }

    companion object {
        @Suppress("UNUSED_PARAMETER")
        fun reverseResponse(
            data: Message,
            r: Response
        ) {
            r("ok")
        }

        fun deferredReverseResponse(
            data: Message,
            r: Response
        ) {
            Thread {
                Thread.sleep(50)
                r("ok")
            }.start()
        }

        fun deferredAnswer(
            data: Message,
            r: Response
        ) {
            Thread {
                Thread.sleep(50)
                r("deferred")
            }.start()
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

    // The jni boundary clears anything the callback raises, so a failed check is captured and raised here.
    @Suppress("TooGenericExceptionCaught")
    private inline fun <reified T> verifyCall(
        request: Request,
        crossinline block: (T?) -> Unit
    ) {
        var failure: Throwable? = null
        var answered = false

        Client.call<T>(request) { response ->
            answered = true

            try {
                block(response)
            } catch (e: Throwable) {
                failure = e
            }
        }

        assertTrue(answered, "the callback was never invoked")
        failure?.let { throw it }
    }

    @Suppress("TooGenericExceptionCaught")
    private fun verifyCall(
        requestData: String,
        block: (String) -> Unit
    ) {
        var failure: Throwable? = null
        var answered = false

        Client.call(requestData) { response ->
            answered = true

            try {
                block(response)
            } catch (e: Throwable) {
                failure = e
            }
        }

        assertTrue(answered, "the callback was never invoked")
        failure?.let { throw it }
    }

    @Test
    fun callAsyncGivesTheSchedulerATurnBeforeAnswering() = runBlocking {
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
    fun batteryLevelAsync() {
        setUp()

        MappingList.add(
            "platform.battery.level",
            MappingItem(
                ClientTest::batteryLevel
            )
        )

        val request = Request("platform.battery.level", Param("suffix", "%"))

        runBlocking {
            verifyCall<String>(request) { response ->
                assertEquals("100%", response)
            }
        }
    }

    @Test
    fun batteryLevelCallAsync() =
        runTest {
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

        verifyCall<Boolean>(request) { response ->
            assertEquals(false, response)
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
            verifyCall<String>(request) { response ->
                assertEquals("LOGGED-WITH-REMEMBER", response)
            }
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

        verifyCall<Boolean>(request) { response ->
            assertEquals(false, response)
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

        // The kotlin side catches an exception and answers, so only an error reaches the jni boundary, which has to report it and answer rather than leave the caller waiting.

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
            verifyCall<String>(request) { response ->
                assertEquals("response-is-ok", response)
            }
        }
    }

    // The bridge converts utf16 to utf8 by hand, so a string has to survive every width it can be encoded in.
    @Test
    fun textSurvivesEveryEncodingWidth() {
        setUp()

        val samples = listOf(
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

    // The mapping answers after the call returned, from a thread it created, and still has to reach kotlin.
    @Test
    fun cancellingCallAsyncLeavesNothingRegistered() = runBlocking {
        setUp()

        // A cancelled call has to drop its registration, since nothing else will ever resolve that key.

        val before = CallbackList.count()

        val job = launch {
            Client.callAsync<String>(Request("sample.async"))
        }

        while (CallbackList.count() == before) {
            delay(1)
        }

        job.cancelAndJoin()

        assertEquals(before, CallbackList.count())
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
        val ptr = ByteBufferHelper.getPtrAddress(buffer)

        val dataView = DataView(ptr, data.size)
        val request = Request("sample.image.grayscale.dataview", Param("dataView", dataView))

        verifyCall<String>(request) { response ->
            assertEquals("OK", response)

            assertEquals(16, buffer.capacity())
            assertEquals(buffer[0].toInt(), 85)
            assertEquals(buffer[4].toInt(), 85)
            assertEquals(buffer[8].toInt(), 85)
            assertEquals(buffer[12].toInt(), 0)
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
        val ptr = ByteBufferHelper.getPtrAddress(buffer)

        val dataView = DataView(ptr, data.size)
        val request = Request("sample.image.grayscale.dataview", Param("dataView", dataView))

        runBlocking {
            verifyCall<String>(request) { response ->
                assertEquals("OK", response)

                assertEquals(16, buffer.capacity())
                assertEquals(buffer[0].toInt(), 85)
                assertEquals(buffer[4].toInt(), 85)
                assertEquals(buffer[8].toInt(), 85)
                assertEquals(buffer[12].toInt(), 0)
            }
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
        }
    }

    @Test
    fun dataViewAsync() {
        setUp()

        val request = Request("sample.dataview")

        runBlocking {
            verifyCall<DataView>(request) { response ->
                assertNotNull(response)

                val originalData = ByteArrayHelper.createFromDataView(response)

                assertEquals(16, originalData.size)
                assertEquals(originalData[0].toUByte().toInt(), 255)
                assertEquals(originalData[5].toUByte().toInt(), 255)
                assertEquals(originalData[10].toUByte().toInt(), 255)
                assertEquals(originalData[12].toUByte().toInt(), 0)

                val dataView2 = DataView.createFromByteArray(originalData)
                val request2 =
                    Request("sample.image.grayscale.dataview", Param("dataView", dataView2))

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
            }
        }
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
    fun batteryLevelAsyncFromString() {
        setUp()

        MappingList.add("platform.battery.level", MappingItem(ClientTest::batteryLevel))

        val request = Request("platform.battery.level", Param("suffix", "%"))

        runBlocking {
            verifyCall(request.data) { response ->
                assertEquals("{\"r\":\"100%\"}", response)
            }
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

        verifyCall<String>(request) { response ->
            assertEquals("OK", response)
            assertEquals(size, dataView.size)
            assertEquals(100, buffer[0].toInt())
            assertEquals(100, buffer[size - 1].toInt())
        }
    }
    // Nothing owns this name, so the caller is answered with the empty value and the registration is not left behind.
    @Test
    fun nestedHostMappingAnsweringLaterStillReachesTheOuterCaller() {
        setUp()

        MappingList.add(
            "platform.reverse.response",
            MappingItem(
                ClientTest::deferredReverseResponse
            )
        )

        val answered = CountDownLatch(1)
        var response: String? = null

        Client.call<String>(Request("sample.reverse")) {
            response = it
            answered.countDown()
        }

        assertTrue(answered.await(5, TimeUnit.SECONDS))
        assertEquals("response-is-ok", response)
    }

    @Test
    fun hostMappingAnswersAfterTheCallReturned() {
        setUp()

        MappingList.add(
            "platform.deferred.answer",
            MappingItem(
                ClientTest::deferredAnswer
            )
        )

        val answered = CountDownLatch(1)
        var response: String? = null
        var returnedBeforeAnswer = false

        Client.call<String>(Request("platform.deferred.answer")) {
            response = it
            answered.countDown()
        }

        returnedBeforeAnswer = answered.count > 0

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
    fun unknownFunctionAnswersEmptyAndLeavesNothingRegistered() {
        setUp()

        val before = CallbackList.count()

        verifyCall<String>(Request("not.found")) { response ->
            assertEquals(null, response)
        }

        assertEquals(before, CallbackList.count())
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

        val request = Request(
            "sample.login",
            Param("username", "paulo"),
            Param("password", "123456"),
            Param("remember", true)
        )

        assertEquals("LOGGED-WITH-REMEMBER", Client.callSync<String>(request))
    }

    @Test
    fun callSyncAnswersEmptyWhenTheMappingDefers() {
        // A mapping that answers later cannot be read synchronously, and the registration it would have resolved is dropped rather than left behind.

        setUp()

        MappingList.add(
            "platform.deferred.answer",
            MappingItem(
                ClientTest::deferredAnswer
            )
        )

        val before = CallbackList.count()

        assertEquals(null, Client.callSync<String>(Request("platform.deferred.answer")))
        assertEquals(before, CallbackList.count())

        // The mapping answers into a registration that is already gone, and the suite waits for it rather than leaving it running past the process.
        Thread.sleep(200)

        assertEquals(before, CallbackList.count())
    }

    @Test
    fun callSyncAnswersEmptyWhenNothingOwnsTheFunction() {
        setUp()

        val before = CallbackList.count()

        assertEquals(null, Client.callSync<String>(Request("not.found")))
        assertEquals(before, CallbackList.count())
    }

    @Test
    fun callSyncAnswersTheDocumentFromAString() {
        setUp()

        val request = "{\"f\":\"sample.login\",\"p\":[{\"n\":\"username\",\"v\":\"paulo\"}," +
            "{\"n\":\"password\",\"v\":\"123456\"},{\"n\":\"remember\",\"v\":true}]}"

        assertEquals("{\"r\":\"LOGGED-WITH-REMEMBER\"}", Client.callSync(request))
    }

    @Test
    fun callSyncAnswersEmptyFromAStringWhenTheMappingDefers() {
        setUp()

        MappingList.add(
            "platform.deferred.answer",
            MappingItem(
                ClientTest::deferredAnswer
            )
        )

        val before = CallbackList.count()

        assertEquals("", Client.callSync("{\"f\":\"platform.deferred.answer\",\"p\":[]}"))
        assertEquals(before, CallbackList.count())

        Thread.sleep(200)
    }

    @Test
    fun callSyncAnswersEmptyFromAStringWhenNothingOwnsTheFunction() {
        setUp()

        val before = CallbackList.count()

        assertEquals("", Client.callSync("{\"f\":\"not.found\",\"p\":[]}"))
        assertEquals(before, CallbackList.count())
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

        val samples = mapOf(
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
}
