package com.xplpc.message

import com.xplpc.util.Log

class Message {
    @PublishedApi
    internal val values = HashMap<String, Any>()

    fun set(name: String, value: Any) {
        values[name] = value
    }

    inline fun <reified T> get(name: String): T? {
        // The type is reified so the stored value is really checked, since an erased cast would only fail later at the call site.

        val stored = values[name] ?: return null

        if (stored is T) {
            return stored
        }

        Log.e("[Message : get] Value of \"$name\" has another type")

        return null
    }
}
