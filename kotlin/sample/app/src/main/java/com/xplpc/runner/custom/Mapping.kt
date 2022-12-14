package com.xplpc.runner.custom

import com.xplpc.data.MappingData
import com.xplpc.map.MappingItem

object Mapping {
    fun initialize() {
        MappingData.add(MappingItem("platform.battery.level", Callback::batteryLevel))
    }
}
