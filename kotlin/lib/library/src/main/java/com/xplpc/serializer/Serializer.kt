package com.xplpc.serializer

import com.google.gson.reflect.TypeToken
import com.xplpc.message.Message
import com.xplpc.message.Param

interface Serializer {
    fun encodeRequest(functionName: String, vararg params: Param): String
    fun decodeFunctionName(data: String): String
    fun <T> decodeFunctionReturnValue(data: String, type: TypeToken<T>): T?
    fun decodeMessage(data: String): Message?
    fun encodeFunctionReturnValue(data: Any): String
}
