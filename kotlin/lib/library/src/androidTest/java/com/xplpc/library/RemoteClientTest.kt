package com.xplpc.library

import androidx.test.ext.junit.runners.AndroidJUnit4
import com.xplpc.client.RemoteClient
import com.xplpc.core.Config
import com.xplpc.core.XPLPC
import com.xplpc.data.MappingList
import com.xplpc.map.MappingItem
import com.xplpc.message.Message
import com.xplpc.message.Param
import com.xplpc.message.Request
import com.xplpc.message.Response
import com.xplpc.serializer.JsonSerializer
import kotlinx.coroutines.runBlocking
import org.junit.Assert.assertEquals
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith

@RunWith(AndroidJUnit4::class)
class RemoteClientTest {
    companion object {
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
}
