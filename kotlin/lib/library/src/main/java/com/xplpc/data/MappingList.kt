package com.xplpc.data

import com.xplpc.map.MappingItem

object MappingList {
    private val list = HashMap<String, MappingItem>()

    fun add(name: String, item: MappingItem) {
        list[name] = item
    }

    fun find(name: String): MappingItem? {
        return list[name]
    }
}
