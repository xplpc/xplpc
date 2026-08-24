package com.xplpc.data

import java.util.concurrent.ConcurrentHashMap

object CallbackList {
    private val list = ConcurrentHashMap<String, (String) -> Unit>()

    fun add(key: String, callback: (String) -> Unit) {
        list[key] = callback
    }

    fun execute(key: String, data: String) {
        // The entry is removed and invoked in a single step, so concurrent callers never run the same callback twice.

        list.remove(key)?.invoke(data)
    }

    fun remove(key: String) {
        list.remove(key)
    }

    fun clear() {
        list.clear()
    }

    fun count(): Int {
        return list.size
    }
}
