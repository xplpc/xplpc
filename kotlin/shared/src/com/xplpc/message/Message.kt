package com.xplpc.message

import com.google.gson.reflect.TypeToken
import com.xplpc.core.XPLPC
import com.xplpc.serializer.Nullability
import com.xplpc.util.Log
import kotlin.reflect.typeOf

class Message {
    @PublishedApi
    internal val values = HashMap<String, Any>()

    fun set(
        name: String,
        value: Any
    ) {
        values[name] = value
    }

    inline fun <reified T> get(name: String): T? {
        // A value keeps the form it arrived in until a mapping asks for a type, so it is converted against that type rather than against whatever a generic decode guessed.

        val stored = values[name] ?: return null
        val value = XPLPC.config.serializer.decodeValue(stored, object : TypeToken<T>() {})

        // The jvm erases what a type says about null inside a list or a map, so a value holding null where its type holds none is read as one of another type.
        if (value != null && Nullability.holds(value, typeOf<T>())) {
            return value
        }

        Log.e("[Message : get] Value of \"$name\" has another type")

        return null
    }
}
