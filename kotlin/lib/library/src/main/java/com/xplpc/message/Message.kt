package com.xplpc.message

import com.xplpc.util.Log

@Suppress("UNCHECKED_CAST")
class Message : HashMap<String, Any>() {
    fun <T> value(name: String): T? {
        try {
            return get(name) as T
        } catch (e: Exception) {
            Log.e("[Message : value] Error when cast value: ${e.message}")
        }

        return null
    }
}
