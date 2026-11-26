package com.xplpc.data

import com.xplpc.layer.Layer
import com.xplpc.map.MappingItem
import java.util.concurrent.ConcurrentHashMap

object MappingList {
    private val list = ConcurrentHashMap<String, MappingItem>()

    // The mapping is recorded before the name is declared, so a call routed the moment the router hears of it already finds it.
    @Synchronized
    fun add(
        name: String,
        item: MappingItem
    ) {
        val previous = list.put(name, item)

        try {
            Layer.declare(name)
        } catch (e: Throwable) {
            if (previous == null) {
                list.remove(name)
            } else {
                list[name] = previous
            }

            throw e
        }
    }

    fun find(name: String): MappingItem? = list[name]

    @Synchronized
    fun clear() {
        for (name in list.keys) {
            Layer.retract(name)
        }

        list.clear()
    }

    fun count(): Int = list.size

    fun has(name: String): Boolean = list.containsKey(name)
}
