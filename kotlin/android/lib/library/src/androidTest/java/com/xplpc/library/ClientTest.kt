package com.xplpc.library

import android.content.Context
import android.os.BatteryManager
import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.test.platform.app.InstrumentationRegistry
import com.xplpc.client.Client
import com.xplpc.data.MappingList
import com.xplpc.helper.ByteArrayHelper
import com.xplpc.helper.ByteBufferHelper
import com.xplpc.map.MappingItem
import com.xplpc.message.Message
import com.xplpc.message.Param
import com.xplpc.message.Request
import com.xplpc.message.Response
import com.xplpc.type.DataView
import kotlinx.coroutines.runBlocking
import kotlinx.coroutines.test.runTest
import org.junit.Assert.assertEquals
import org.junit.Assert.assertNotNull
import org.junit.Test
import org.junit.runner.RunWith
import java.nio.ByteBuffer

@RunWith(AndroidJUnit4::class)
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
            val appContext = InstrumentationRegistry.getInstrumentation().targetContext
            val bm = appContext.getSystemService(Context.BATTERY_SERVICE) as BatteryManager
            val level = bm.getIntProperty(BatteryManager.BATTERY_PROPERTY_CAPACITY)

            val suffix = data.value<String>("suffix")

            if (suffix != null) {
                r("$level$suffix")
            } else {
                r("$level")
            }
        }
    }

    @Test
    fun batteryLevel() {
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
        // get data view
        val request = Request("sample.dataview")

        Client.call<DataView>(request) { response ->
            // check response
            assertNotNull(response)

            // check current values
            val dataView = response!!
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
        // get data view
        val request = Request("sample.dataview")

        runBlocking {
            Client.call<DataView>(request) { response ->
                // check response
                assertNotNull(response)

                // check current values
                val dataView = response!!
                val originalData = ByteArrayHelper.createFromDataView(dataView)

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
        MappingList.add("platform.battery.level", MappingItem(ClientTest::batteryLevel))

        val request = Request("platform.battery.level", Param("suffix", "%"))

        runBlocking {
            Client.call(request.data) { response ->
                assertEquals("{\"r\":\"100%\"}", response)
            }
        }
    }
}
