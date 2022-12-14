package com.xplpc.runner.custom

import android.content.Context.BATTERY_SERVICE
import android.os.BatteryManager
import com.xplpc.message.Message
import com.xplpc.message.Response
import com.xplpc.runner.app.Application

object Callback {
    fun batteryLevel(data: Message): Response {
        val bm = Application.instance.getSystemService(BATTERY_SERVICE) as BatteryManager
        val level = bm.getIntProperty(BatteryManager.BATTERY_PROPERTY_CAPACITY)

        val suffix = data.value<String>("suffix")

        return if (suffix != null) {
            Response("$level$suffix")
        } else {
            Response("$level")
        }
    }
}
