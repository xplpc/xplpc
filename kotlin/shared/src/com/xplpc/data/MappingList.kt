package com.xplpc.data

import com.xplpc.map.MappingItem
import java.util.concurrent.ConcurrentHashMap

object MappingList {
    private val list = ConcurrentHashMap<String, MappingItem>()

    fun add(name: String, item: MappingItem) {
        list[name] = item
    }

    fun find(name: String): MappingItem? {
        return list[name]
    }

    fun clear() {
        list.clear()
    }

    fun count(): Int {
        return list.size
    }

    fun has(name: String): Boolean {
        return list.containsKey(name)
    }
}
