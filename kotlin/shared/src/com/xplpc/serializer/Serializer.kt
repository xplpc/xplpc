package com.xplpc.serializer

import com.google.gson.reflect.TypeToken
import com.xplpc.message.DecodedRequest
import com.xplpc.message.Param

interface Serializer {
    fun encodeRequest(
        functionName: String,
        vararg params: Param
    ): String

    fun <T> decodeFunctionReturnValue(
        data: String,
        type: TypeToken<T>
    ): T?

    fun decodeRequest(data: String): DecodedRequest?

    fun encodeFunctionReturnValue(data: Any?): String

    // Answers the value as the requested type, or null when it does not hold one exactly.
    fun <T> decodeValue(
        value: Any,
        type: TypeToken<T>
    ): T?
}
