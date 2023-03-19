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

    fun clear() {
        list.clear()
    }

    fun has(name: String): Boolean {
        return find(name) != null
    }
}
