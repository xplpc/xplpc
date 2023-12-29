package com.xplpc.library

import com.xplpc.client.Client
import com.xplpc.data.MappingList
import com.xplpc.helper.ByteArrayHelper
import com.xplpc.helper.ByteBufferHelper
import com.xplpc.map.MappingItem
import com.xplpc.message.Message
import com.xplpc.message.Param
import com.xplpc.message.Request
import com.xplpc.message.Response
import com.xplpc.platform.PlatformInitializer
import com.xplpc.type.DataView
import kotlinx.coroutines.runBlocking
import kotlinx.coroutines.test.runTest
import java.nio.ByteBuffer
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertNotNull

class ClientTest {
    companion object {
        @Suppress("UNUSED_PARAMETER")
        fun reverseResponse(
            data: Message,
            r: Response
        ) {
            r("ok")
        }

        fun batteryLevel(
            data: Message,
            r: Response
        ) {
            val level = 100
            val suffix = data.value<String>("suffix")

            if (suffix != null) {
                r("$level$suffix")
            } else {
                r("$level")
            }
        }
    }

    private fun setUp() {
        PlatformInitializer.initialize()
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

        Client.call<String>(request) { response ->
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
            Client.call<String>(request) { response ->
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

        Client.call<Boolean>(request) { response ->
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

        Client.call<String>(request) { response ->
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
            Client.call<String>(request) { response ->
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

        Client.call<Boolean>(request) { response ->
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

        Client.call<String>(request) { response ->
            assertEquals("response-is-ok", response)
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
            Client.call<String>(request) { response ->
                assertEquals("response-is-ok", response)
            }
        }
    }

    @Test
    fun grayscaleImageWithDataView() {
        setUp()

        // in kotlin the 255 byte value is -1
        val data =
            byteArrayOf(
                // red pixel
                -1, 0, 0, -1,
                // green pixel
                0, -1, 0, -1,
                // blue pixel
                0, 0, -1, -1,
                // transparent pixel
                0, 0, 0, 0,
            )

        val buffer: ByteBuffer = ByteBuffer.allocateDirect(data.size)
        buffer.put(data)
        val ptr = ByteBufferHelper.getPtrAddress(buffer)

        val dataView = DataView(ptr, data.size)
        val request = Request("sample.image.grayscale.dataview", Param("dataView", dataView))

        Client.call<String>(request) { response ->
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

        // in kotlin the 255 byte value is -1
        val data =
            byteArrayOf(
                // red pixel
                -1, 0, 0, -1,
                // green pixel
                0, -1, 0, -1,
                // blue pixel
                0, 0, -1, -1,
                // transparent pixel
                0, 0, 0, 0,
            )

        val buffer: ByteBuffer = ByteBuffer.allocateDirect(data.size)
        buffer.put(data)
        val ptr = ByteBufferHelper.getPtrAddress(buffer)

        val dataView = DataView(ptr, data.size)
        val request = Request("sample.image.grayscale.dataview", Param("dataView", dataView))

        runBlocking {
            Client.call<String>(request) { response ->
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

        // get data view
        val request = Request("sample.dataview")

        Client.call<DataView>(request) { response ->
            // check response
            assertNotNull(response)

            // check current values
            val dataView = response
            val originalData = ByteArrayHelper.createFromDataView(dataView)

            assertEquals(16, originalData.size)
            assertEquals(originalData[0].toUByte().toInt(), 255)
            assertEquals(originalData[5].toUByte().toInt(), 255)
            assertEquals(originalData[10].toUByte().toInt(), 255)
            assertEquals(originalData[12].toUByte().toInt(), 0)

            // send original data and check modified data
            val dataView2 = DataView.createFromByteArray(originalData)
            val request2 = Request("sample.image.grayscale.dataview", Param("dataView", dataView2))

            Client.call<String>(request2) { response2 ->
                assertEquals("OK", response2)

                val processedData = ByteArrayHelper.createFromDataView(dataView2)

                // check copied values
                assertEquals(16, processedData.size)
                assertEquals(processedData[0].toUByte().toInt(), 85)
                assertEquals(processedData[5].toUByte().toInt(), 85)
                assertEquals(processedData[10].toUByte().toInt(), 85)
                assertEquals(processedData[12].toUByte().toInt(), 0)

                // check original values again
                assertEquals(16, originalData.size)
                assertEquals(originalData[0].toUByte().toInt(), 85)
                assertEquals(originalData[5].toUByte().toInt(), 85)
                assertEquals(originalData[10].toUByte().toInt(), 85)
                assertEquals(originalData[12].toUByte().toInt(), 0)
            }
        }
    }

    @Test
    fun dataViewAsync() {
        setUp()

        // get data view
        val request = Request("sample.dataview")

        runBlocking {
            Client.call<DataView>(request) { response ->
                // check response
                assertNotNull(response)

                // check current values
                val originalData = ByteArrayHelper.createFromDataView(response)

                assertEquals(16, originalData.size)
                assertEquals(originalData[0].toUByte().toInt(), 255)
                assertEquals(originalData[5].toUByte().toInt(), 255)
                assertEquals(originalData[10].toUByte().toInt(), 255)
                assertEquals(originalData[12].toUByte().toInt(), 0)

                // send original data and check modified data
                val dataView2 = DataView.createFromByteArray(originalData)
                val request2 =
                    Request("sample.image.grayscale.dataview", Param("dataView", dataView2))

                Client.call<String>(request2) { response2 ->
                    assertEquals("OK", response2)

                    val processedData = ByteArrayHelper.createFromDataView(dataView2)

                    // check copied values
                    assertEquals(16, processedData.size)
                    assertEquals(processedData[0].toUByte().toInt(), 85)
                    assertEquals(processedData[5].toUByte().toInt(), 85)
                    assertEquals(processedData[10].toUByte().toInt(), 85)
                    assertEquals(processedData[12].toUByte().toInt(), 0)

                    // check original values again
                    assertEquals(16, originalData.size)
                    assertEquals(originalData[0].toUByte().toInt(), 85)
                    assertEquals(originalData[5].toUByte().toInt(), 85)
                    assertEquals(originalData[10].toUByte().toInt(), 85)
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

        Client.call(request.data) { response ->
            assertEquals("{\"r\":\"100%\"}", response)
        }
    }

    @Test
    fun batteryLevelAsyncFromString() {
        setUp()

        MappingList.add("platform.battery.level", MappingItem(ClientTest::batteryLevel))

        val request = Request("platform.battery.level", Param("suffix", "%"))

        runBlocking {
            Client.call(request.data) { response ->
                assertEquals("{\"r\":\"100%\"}", response)
            }
        }
    }
}
