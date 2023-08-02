package com.xplpc.data

import java.util.concurrent.ConcurrentHashMap

object CallbackList {
    private val list = ConcurrentHashMap<String, (String) -> Unit>()

    fun add(key: String, callback: (String) -> Unit) {
        list[key] = callback
    }

    fun execute(key: String, data: String) {
        list[key]?.let { callback ->
            list.remove(key)
            callback(data)
        }
    }

    fun count(): Int {
        return list.count()
    }
}
