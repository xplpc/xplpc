package com.xplpc.library

import androidx.test.ext.junit.runners.AndroidJUnit4
import com.xplpc.client.RemoteClient
import com.xplpc.core.Config
import com.xplpc.core.XPLPC
import com.xplpc.message.Param
import com.xplpc.message.Request
import com.xplpc.serializer.JsonSerializer
import kotlinx.coroutines.runBlocking
import org.junit.Assert.assertEquals
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith

@RunWith(AndroidJUnit4::class)
class RemoteClientTest {
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

        val response = RemoteClient.call<String>(request) ?: "ERROR"

        assertEquals("LOGGED-WITH-REMEMBER", response)
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
            val response = RemoteClient.callAsync<String>(request).await() ?: "ERROR"
            assertEquals("LOGGED-WITH-REMEMBER", response)
        }
    }
}
