package com.xplpc.serializer

import com.google.gson.Gson
import com.google.gson.GsonBuilder
import com.google.gson.JsonDeserializationContext
import com.google.gson.JsonDeserializer
import com.google.gson.JsonElement
import com.google.gson.JsonObject
import com.google.gson.JsonPrimitive
import com.google.gson.JsonSerializationContext
import com.google.gson.reflect.TypeToken
import com.xplpc.message.DecodedRequest
import com.xplpc.message.Message
import com.xplpc.message.Param
import com.xplpc.type.DataView
import com.xplpc.util.Log
import java.lang.reflect.Type
import java.util.Date

class JsonSerializer : Serializer {
    // Gson is immutable and thread safe once built, so the adapters are registered a single time.
    private val gson: Gson by lazy { createGson() }

    // Gson fills these by reflection and leaves a missing field null, whatever the declared type says.
    @Suppress("UNUSED")
    internal class JsonRequestData(val f: String?, val p: ArrayList<Param> = ArrayList())

    @Suppress("UNUSED")
    internal class JsonFunctionReturnValueData<T>(val r: T)

    override fun encodeRequest(functionName: String, vararg params: Param): String {
        try {
            val request = JsonRequestData(functionName)

            for (p in params) {
                request.p.add(p)
            }

            return gson.toJson(request)
        } catch (e: Exception) {
            Log.e("[JsonSerializer : encodeRequest] Error when encode data")
            Log.d("[JsonSerializer : encodeRequest] Error when encode data: ${e.message}")
        }

        return ""
    }

    override fun <T> decodeFunctionReturnValue(data: String, type: TypeToken<T>): T? {
        // An empty response is the empty value every failing path answers with, not a document that failed to parse.
        if (data.isEmpty()) {
            return null
        }

        try {
            val typeToken = TypeToken.getParameterized(
                JsonFunctionReturnValueData::class.java, type.type
            )
            return gson.fromJson<JsonFunctionReturnValueData<T>>(data, typeToken.type).r
        } catch (e: Exception) {
            Log.e("[JsonSerializer : decodeFunctionReturnValue] Error when parse json")
            Log.d("[JsonSerializer : decodeFunctionReturnValue] Error when parse json: ${e.message}")
        }

        return null
    }

    override fun encodeFunctionReturnValue(data: Any?): String {
        try {
            val obj = JsonFunctionReturnValueData(data)
            return gson.toJson(obj, JsonFunctionReturnValueData::class.java)
        } catch (e: Exception) {
            Log.e("[JsonSerializer : encodeFunctionReturnValue] Error when encode data")
            Log.d("[JsonSerializer : encodeFunctionReturnValue] Error when encode data: ${e.message}")
        }

        return ""
    }

    override fun decodeRequest(data: String): DecodedRequest? {
        try {
            val decodedData = gson.fromJson(data, JsonRequestData::class.java)

            if (decodedData == null) {
                Log.e("[JsonSerializer : decodeRequest] Error when decode request")
                return null
            }
            val message = Message()

            // A parameter that arrived without a value is left out, so reading it answers null like every other bridge.
            for (p in decodedData.p.orEmpty()) {
                p.v?.let { message.set(p.n, it) }
            }

            return DecodedRequest(decodedData.f.orEmpty(), message)
        } catch (e: Exception) {
            Log.e("[JsonSerializer : decodeRequest] Error when decode request")
            Log.d("[JsonSerializer : decodeRequest] Error when decode request: ${e.message}")
        }

        return null
    }

    private fun createGson(): Gson {
        // A parameter without a value has to reach the wire as null, and gson drops it from the object otherwise.
        val builder = GsonBuilder().serializeNulls()

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
                        Log.e("[JsonSerializer : deserialize] Error when read a date")
                        Log.d("[JsonSerializer : deserialize] Error when read a date: ${e.message}")
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
                        Log.e("[JsonSerializer : deserialize] Error when read a character")
                        Log.d("[JsonSerializer : deserialize] Error when read a character: ${e.message}")
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
        builder.registerTypeAdapter(Char::class.javaObjectType, customCharSerializer)

        val customDataViewSerializer =
            object : JsonDeserializer<Any?>, com.google.gson.JsonSerializer<DataView?> {
                override fun deserialize(
                    json: JsonElement,
                    typeOfT: Type?,
                    context: JsonDeserializationContext?
                ): Any? {
                    return try {
                        val obj = json.asJsonObject
                        val ptr = obj["ptr"].asLong
                        val size = obj["size"].asInt

                        DataView(ptr, size)
                    } catch (e: Exception) {
                        Log.e("[JsonSerializer : deserialize] Error when read a data view")
                        Log.d("[JsonSerializer : deserialize] Error when read a data view: ${e.message}")
                        null
                    }
                }

                override fun serialize(
                    src: DataView?,
                    typeOfSrc: Type?,
                    context: JsonSerializationContext?
                ): JsonElement? {
                    return src?.let {
                        val obj = JsonObject()
                        obj.addProperty("ptr", it.ptr)
                        obj.addProperty("size", it.size)
                        return obj
                    }
                }
            }

        builder.registerTypeAdapter(DataView::class.java, customDataViewSerializer)

        return builder.create()
    }
}
