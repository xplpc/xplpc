package com.xplpc.data

import com.xplpc.map.MappingItem

object MappingData {
    private val list = ArrayList<MappingItem>()

    fun add(item: MappingItem) {
        list.add(item)
    }

    fun find(name: String): MappingItem? {
        return list.find { it.name == name }
    }
}
