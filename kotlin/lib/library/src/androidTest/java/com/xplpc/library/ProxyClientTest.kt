package com.xplpc.library

import android.content.Context
import android.os.BatteryManager
import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.test.platform.app.InstrumentationRegistry
import com.xplpc.client.ProxyClient
import com.xplpc.core.Config
import com.xplpc.core.XPLPC
import com.xplpc.data.MappingData
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
class ProxyClientTest {
    companion object {
        fun batteryLevel(data: Message): Response {
            val appContext = InstrumentationRegistry.getInstrumentation().targetContext
            val bm = appContext.getSystemService(Context.BATTERY_SERVICE) as BatteryManager
            val level = bm.getIntProperty(BatteryManager.BATTERY_PROPERTY_CAPACITY)

            val suffix = data.value<String>("suffix")

            return if (suffix != null) {
                Response("$level$suffix")
            } else {
                Response("$level")
            }
        }
    }

    @Before
    fun setUp() {
        XPLPC.initialize(
            Config(JsonSerializer()),
        )
    }

    @Test
    fun batteryLevel() {
        MappingData.add(
            MappingItem(
                "platform.battery.level", ProxyClientTest::batteryLevel
            )
        )

        val request = Request(
            "platform.battery.level", Param("suffix", "%")
        )

        val response = ProxyClient.call(request.data)

        assertEquals("{\"r\":\"100%\"}", response)
    }

    @Test
    fun batteryLevelAsync() {
        MappingData.add(
            MappingItem(
                "platform.battery.level", ProxyClientTest::batteryLevel
            )
        )

        val request = Request(
            "platform.battery.level", Param("suffix", "%")
        )

        runBlocking {
            val response = ProxyClient.callAsync(request.data).await()
            assertEquals("{\"r\":\"100%\"}", response)
        }
    }
}
