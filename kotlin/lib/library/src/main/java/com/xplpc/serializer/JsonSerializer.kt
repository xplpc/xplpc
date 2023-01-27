package com.xplpc.serializer

import com.google.gson.Gson
import com.google.gson.GsonBuilder
import com.google.gson.JsonDeserializationContext
import com.google.gson.JsonDeserializer
import com.google.gson.JsonElement
import com.google.gson.JsonPrimitive
import com.google.gson.JsonSerializationContext
import com.google.gson.reflect.TypeToken
import com.xplpc.message.Message
import com.xplpc.message.Param
import com.xplpc.util.Log
import java.lang.reflect.Type
import java.util.Date

class JsonSerializer : Serializer {
    @Suppress("UNUSED")
    internal class JsonRequestData(val f: String, val p: ArrayList<Param> = ArrayList())

    @Suppress("UNUSED")
    internal class JsonFunctionNameData(val f: String)

    @Suppress("UNUSED")
    internal class JsonFunctionReturnValueData<T>(val r: T)

    @Suppress("UNUSED")
    internal class JsonParametersData(val p: ArrayList<Param> = ArrayList())

    override fun encodeRequest(functionName: String, vararg params: Param): String {
        try {
            val request = JsonRequestData(functionName)

            for (p in params) {
                request.p.add(p)
            }

            val gson = createGson()
            return gson.toJson(request)
        } catch (e: Exception) {
            Log.e("[JsonSerializer : encodeRequest] Error when encode data: ${e.message}")
        }

        return ""
    }

    override fun decodeFunctionName(data: String): String {
        try {
            val gson = createGson()
            return gson.fromJson(data, JsonFunctionNameData::class.java).f
        } catch (e: Exception) {
            Log.e("[JsonSerializer : decodeFunctionName] Error when parse json: ${e.message}")
        }

        return ""
    }

    @Suppress("UNCHECKED_CAST")
    override fun <T> decodeFunctionReturnValue(data: String, type: TypeToken<T>): T? {
        try {
            val gson = createGson()
            val typeToken = TypeToken.getParameterized(
                JsonFunctionReturnValueData::class.java, type.type
            )
            return gson.fromJson<JsonFunctionReturnValueData<T>>(data, typeToken.type).r
        } catch (e: Exception) {
            Log.e("[JsonSerializer : decodeFunctionReturnValue] Error when parse json: ${e.message}")
        }

        return null
    }

    @Suppress("UNCHECKED_CAST")
    override fun encodeFunctionReturnValue(data: Any): String {
        try {
            val obj = JsonFunctionReturnValueData(data)
            val gson = createGson()
            return gson.toJson(obj, JsonFunctionReturnValueData::class.java)
        } catch (e: Exception) {
            Log.e("[JsonSerializer : encodeFunctionReturnValue] Error when encode data: ${e.message}")
        }

        return ""
    }

    override fun decodeMessage(data: String): Message? {
        try {
            var decodedData: JsonParametersData? = null

            // decode parameters
            try {
                val gson = createGson()
                decodedData = gson.fromJson(data, JsonParametersData::class.java)
            } catch (e: Exception) {
                Log.e("[JsonSerializer : decodeMessage] Error when get parameters data: ${e.message}")
            }

            if (decodedData == null) {
                return null
            }

            // message data
            val message = Message()

            for (p in decodedData.p) {
                message[p.n] = p.v
            }

            return message
        } catch (e: Exception) {
            Log.e("[JsonSerializer : decodeMessage] Error when decode message: ${e.message}")
        }

        return null
    }

    private fun createGson(): Gson {
        val builder = GsonBuilder()

        // date
        val customDateSerializer =
            object : JsonDeserializer<Any?>, com.google.gson.JsonSerializer<Date?> {
                override fun deserialize(
                    json: JsonElement,
                    typeOfT: Type?,
                    context: JsonDeserializationContext?
                ): Any? {
                    return try {
                        Date(json.asJsonPrimitive.asLong)
                    } catch (e: Exception) {
                        null
                    }
                }

                override fun serialize(
                    src: Date?,
                    typeOfSrc: Type?,
                    context: JsonSerializationContext?
                ): JsonElement? {
                    return src?.let { JsonPrimitive(it.time) }
                }
            }

        builder.registerTypeAdapter(Date::class.java, customDateSerializer)

        // char
        val customCharSerializer =
            object : JsonDeserializer<Any?>, com.google.gson.JsonSerializer<Char?> {
                override fun deserialize(
                    json: JsonElement,
                    typeOfT: Type?,
                    context: JsonDeserializationContext?
                ): Any? {
                    return try {
                        Char(json.asJsonPrimitive.asInt)
                    } catch (e: Exception) {
                        null
                    }
                }

                override fun serialize(
                    src: Char?,
                    typeOfSrc: Type?,
                    context: JsonSerializationContext?
                ): JsonElement? {
                    return src?.let { JsonPrimitive(it.code) }
                }
            }

        builder.registerTypeAdapter(Char::class.java, customCharSerializer)
        builder.registerTypeAdapter(Character::class.java, customCharSerializer)

        return builder.create()
    }
}
