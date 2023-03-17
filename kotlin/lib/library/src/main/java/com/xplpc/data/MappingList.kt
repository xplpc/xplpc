package com.xplpc.data

import com.xplpc.map.MappingItem

class MappingList private constructor() {
    companion object {
        private val list = HashMap<String, MappingItem>()

        @JvmStatic
        fun add(name: String, item: MappingItem) {
            list[name] = item
        }

        @JvmStatic
        fun find(name: String): MappingItem? {
            return list[name]
        }

        @JvmStatic
        fun clear() {
            list.clear()
        }

        @JvmStatic
        fun has(name: String): Boolean {
            return find(name) != null
        }
    }
}
