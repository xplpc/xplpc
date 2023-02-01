package com.xplpc.library

import androidx.test.ext.junit.runners.AndroidJUnit4
import com.xplpc.client.RemoteClient
import com.xplpc.core.Config
import com.xplpc.core.XPLPC
import com.xplpc.data.MappingList
import com.xplpc.helper.ByteBufferHelper
import com.xplpc.map.MappingItem
import com.xplpc.message.Message
import com.xplpc.message.Param
import com.xplpc.message.Request
import com.xplpc.message.Response
import com.xplpc.serializer.JsonSerializer
import com.xplpc.type.DataView
import kotlinx.coroutines.runBlocking
import org.junit.Assert.assertEquals
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith
import java.nio.ByteBuffer

@RunWith(AndroidJUnit4::class)
class RemoteClientTest {
    companion object {
        @Suppress("UNUSED_PARAMETER")
        fun reverseResponse(data: Message, r: Response) {
            r("ok")
        }
    }

    @Before
    fun setUp() {
        XPLPC.initialize(
            Config(JsonSerializer()),
        )
    }

    @Test
    fun login() {
        val request = Request(
            "sample.login",
            Param("username", "paulo"),
            Param("password", "123456"),
            Param("remember", true)
        )

        RemoteClient.call<String>(request) { response ->
            assertEquals("LOGGED-WITH-REMEMBER", response)
        }
    }

    @Test
    fun loginAsync() {
        val request = Request(
            "sample.login",
            Param("username", "paulo"),
            Param("password", "123456"),
            Param("remember", true)
        )

        runBlocking {
            RemoteClient.call<String>(request) { response ->
                assertEquals("LOGGED-WITH-REMEMBER", response)
            }
        }
    }

    @Test
    fun loginInvalidCast() {
        val request = Request(
            "sample.login",
            Param("username", "paulo"),
            Param("password", "123456"),
            Param("remember", true)
        )

        RemoteClient.call<Boolean>(request) { response ->
            assertEquals(false, response)
        }
    }

    @Test
    fun reverse() {
        MappingList.add(
            "platform.reverse.response",
            MappingItem(
                RemoteClientTest::reverseResponse
            )
        )

        val request = Request("sample.reverse")

        RemoteClient.call<String>(request) { response ->
            assertEquals("response-is-ok", response)
        }
    }

    @Test
    fun reverseAsync() {
        MappingList.add(
            "platform.reverse.response",
            MappingItem(
                RemoteClientTest::reverseResponse
            )
        )

        val request = Request("sample.reverse")

        runBlocking {
            RemoteClient.call<String>(request) { response ->
                assertEquals("response-is-ok", response)
            }
        }
    }

    @Test
    fun dataView() {
        // in kotlin the 255 byte value is -1
        val data = byteArrayOf(
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

        RemoteClient.call<String>(request) { response ->
            assertEquals("OK", response)

            assertEquals(16, buffer.capacity())
            assertEquals(buffer[0].toInt(), 85)
            assertEquals(buffer[4].toInt(), 85)
            assertEquals(buffer[8].toInt(), 85)
            assertEquals(buffer[12].toInt(), 0)
        }
    }

    @Test
    fun dataViewAsync() {
        // in kotlin the 255 byte value is -1
        val data = byteArrayOf(
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
            RemoteClient.call<String>(request) { response ->
                assertEquals("OK", response)

                assertEquals(16, buffer.capacity())
                assertEquals(buffer[0].toInt(), 85)
                assertEquals(buffer[4].toInt(), 85)
                assertEquals(buffer[8].toInt(), 85)
                assertEquals(buffer[12].toInt(), 0)
            }
        }
    }
}
