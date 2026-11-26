package com.xplpc.serializer

import com.google.gson.Gson
import com.google.gson.GsonBuilder
import com.google.gson.JsonArray
import com.google.gson.JsonDeserializationContext
import com.google.gson.JsonDeserializer
import com.google.gson.JsonElement
import com.google.gson.JsonParseException
import com.google.gson.JsonParser
import com.google.gson.JsonPrimitive
import com.google.gson.JsonSerializationContext
import com.google.gson.JsonSyntaxException
import com.google.gson.Strictness
import com.google.gson.ToNumberStrategy
import com.google.gson.reflect.TypeToken
import com.google.gson.stream.JsonReader
import com.google.gson.stream.JsonToken
import com.xplpc.message.DecodedRequest
import com.xplpc.message.Message
import com.xplpc.message.Param
import com.xplpc.type.DataView
import com.xplpc.util.Log
import java.io.IOException
import java.io.StringReader
import java.lang.reflect.Type
import java.math.BigDecimal
import java.math.BigInteger
import java.util.Date
import kotlin.math.abs

class JsonSerializer : Serializer {
    private val integerLiteral = Regex("-?(0|[1-9][0-9]*)")

    // Gson is immutable and thread safe once built, so the adapters are registered a single time.
    private val gson: Gson by lazy { createGson() }

    // Gson writes these by reflection, so the field names are the wire format.
    @Suppress("UNUSED")
    internal class JsonRequestData(
        val f: String,
        val p: ArrayList<Param> = ArrayList()
    )

    @Suppress("UNUSED")
    internal class JsonFunctionReturnValueData<T>(
        val r: T
    )

    override fun encodeRequest(
        functionName: String,
        vararg params: Param
    ): String {
        try {
            val request = JsonRequestData(functionName)

            for (p in params) {
                request.p.add(p)
            }

            return gson.toJson(request)
        } catch (e: Throwable) {
            Log.e("[JsonSerializer : encodeRequest] Error when encode data")
            Log.d("[JsonSerializer : encodeRequest] Error when encode data: ${e.message}")
        }

        return ""
    }

    override fun <T> decodeFunctionReturnValue(
        data: String,
        type: TypeToken<T>
    ): T? {
        // An empty response is the empty value every failing path answers with, not a document that failed to parse.
        if (data.isEmpty()) {
            return null
        }

        val document =
            try {
                parseDocument(data)
            } catch (e: JsonParseException) {
                Log.e("[JsonSerializer : decodeFunctionReturnValue] Error when parse json")
                Log.d("[JsonSerializer : decodeFunctionReturnValue] Error when parse json: ${e.message}")
                return null
            }

        // A document that carries no value is the empty value, which is not a failure to report.
        if (!document.isJsonObject) {
            return null
        }

        val value = document.asJsonObject["r"]

        if (value == null || value.isJsonNull) {
            return null
        }

        val decoded = decodeValue(value, type)

        if (decoded == null) {
            Log.e("[JsonSerializer : decodeFunctionReturnValue] Value has another type")
        }

        return decoded
    }

    override fun encodeFunctionReturnValue(data: Any?): String {
        try {
            val obj = JsonFunctionReturnValueData(data)
            return gson.toJson(obj, JsonFunctionReturnValueData::class.java)
        } catch (e: Throwable) {
            Log.e("[JsonSerializer : encodeFunctionReturnValue] Error when encode data")
            Log.d("[JsonSerializer : encodeFunctionReturnValue] Error when encode data: ${e.message}")
        }

        return ""
    }

    override fun decodeRequest(data: String): DecodedRequest? {
        val document =
            try {
                parseDocument(data)
            } catch (e: JsonParseException) {
                Log.e("[JsonSerializer : decodeRequest] Error when decode request")
                Log.d("[JsonSerializer : decodeRequest] Error when decode request: ${e.message}")
                return null
            }

        if (!document.isJsonObject) {
            Log.e("[JsonSerializer : decodeRequest] Error when decode request")
            return null
        }

        val root = document.asJsonObject

        // A request that names no function as text routes nowhere, which is what the reference refuses before it ever reaches a decoder.
        val functionName = textOf(root["f"])

        if (functionName.isNullOrEmpty()) {
            Log.e("[JsonSerializer : decodeRequest] The request names no function")
            return null
        }

        val params = root["p"]

        if (params == null || params.isJsonNull) {
            return DecodedRequest(functionName, Message())
        }

        if (!params.isJsonArray) {
            Log.e("[JsonSerializer : decodeRequest] Parameters of function \"$functionName\" are not a list")
            return null
        }

        val message = decodeParams(functionName, params.asJsonArray) ?: return null

        return DecodedRequest(functionName, message)
    }

    override fun <T> decodeValue(
        value: Any,
        type: TypeToken<T>
    ): T? {
        // A value set from kotlin is already an instance, so it is only checked against the type.
        if (value !is JsonElement) {
            @Suppress("UNCHECKED_CAST")
            return if (type.rawType.isInstance(value)) value as T else null
        }

        val reader = readers[type.rawType]
        val decoded: Any? = if (reader != null) reader(value) else decodeWithGson(value, type)

        @Suppress("UNCHECKED_CAST")
        return decoded as T?
    }

    // A type json carries directly is read exactly, so anything else is a value of another type the caller reports, while anything that is not a view names no buffer.
    private val readers: Map<Class<*>, (JsonElement) -> Any?> =
        mapOf(
            Int::class.javaObjectType to { value -> exactInteger(value, Int.SIZE_BITS)?.toInt() },
            Long::class.javaObjectType to { value -> exactInteger(value, Long.SIZE_BITS)?.toLong() },
            Short::class.javaObjectType to { value -> exactInteger(value, Short.SIZE_BITS)?.toShort() },
            Byte::class.javaObjectType to { value -> exactInteger(value, Byte.SIZE_BITS)?.toByte() },
            Double::class.javaObjectType to { value -> exactFloating(value, Double.MAX_VALUE) },
            Float::class.javaObjectType to { value -> exactFloating(value, Float.MAX_VALUE.toDouble())?.toFloat() },
            Boolean::class.javaObjectType to { value -> booleanOf(value) },
            String::class.java to { value -> textOf(value) },
            Char::class.javaObjectType to { value -> codePointOf(value) },
            DataView::class.java to { value -> dataViewOf(value) },
        )

    private fun parseDocument(data: String): JsonElement {
        // The other bridges read strict json, and a lenient reader would accept a document none of them can.

        val reader = JsonReader(StringReader(data))
        reader.strictness = Strictness.STRICT

        // The parser reads a document holding nothing but whitespace as null, which every other bridge refuses as unreadable.
        if (peek(reader) == JsonToken.END_DOCUMENT) {
            throw JsonSyntaxException("The document is empty")
        }

        val document = JsonParser.parseReader(reader)

        if (peek(reader) != JsonToken.END_DOCUMENT) {
            throw JsonSyntaxException("The document carries content after its end")
        }

        return document
    }

    // The reader reports a malformed token as an io failure, which is not one the callers above expect.
    private fun peek(reader: JsonReader): JsonToken =
        try {
            reader.peek()
        } catch (e: IOException) {
            throw JsonSyntaxException(e)
        }

    private fun decodeParams(
        functionName: String,
        params: JsonArray
    ): Message? {
        // A name read twice would be answered by whichever copy a bridge happens to keep, so the request is refused rather than guessed at.

        val message = Message()
        val names = HashSet<String>()

        for (param in params) {
            val name = if (param.isJsonObject) textOf(param.asJsonObject["n"]) else null

            if (name == null) {
                Log.e("[JsonSerializer : decodeRequest] Function \"$functionName\" carries a parameter whose name is not text")
                return null
            }

            if (!names.add(name)) {
                Log.e("[JsonSerializer : decodeRequest] Parameter \"$name\" of function \"$functionName\" appears more than once")
                return null
            }

            // A parameter that arrived without a value is left out, so reading it answers null like every other bridge.
            val value = param.asJsonObject["v"]

            if (value != null && !value.isJsonNull) {
                message.set(name, value)
            }
        }

        return message
    }

    private fun <T> decodeWithGson(
        value: JsonElement,
        type: TypeToken<T>
    ): T? =
        try {
            gson.fromJson<T>(value, type.type)
        } catch (e: Throwable) {
            // A consumer type is filled by whatever adapter it carries, which may raise an error as well as an exception.
            Log.d("[JsonSerializer : decodeValue] Error when decode value: ${e.message}")
            null
        }

    private fun exactInteger(
        value: JsonElement,
        bits: Int
    ): BigInteger? = numberLiteralOf(value)?.let { exactInteger(it, bits) }

    private fun exactInteger(
        literal: String,
        bits: Int
    ): BigInteger? {
        // Only an integer literal is an integer, so a fraction is never truncated and a value past the width is refused rather than wrapped.

        if (!integerLiteral.matches(literal)) {
            return null
        }

        val integer = BigInteger(literal)

        return if (integer.bitLength() < bits) integer else null
    }

    private fun exactFloating(
        value: JsonElement,
        greatest: Double
    ): Double? = numberLiteralOf(value)?.let { exactFloating(it, greatest) }

    private fun exactFloating(
        literal: String,
        greatest: Double
    ): Double? {
        // A fraction is rounded to the nearest value the type holds, while an integer has to arrive exactly and a magnitude past the type is refused rather than read as infinity.

        val wide = literal.toDouble()

        if (!wide.isFinite() || abs(wide) > greatest) {
            return null
        }

        val converted = if (greatest == Double.MAX_VALUE) wide else literal.toFloat().toDouble()

        // An exponent can be written wider than a decimal holds, so only an integer literal, which carries none, is compared exactly.
        if (integerLiteral.matches(literal) && BigDecimal(converted).compareTo(BigDecimal(literal)) != 0) {
            return null
        }

        return converted
    }

    private fun numberLiteralOf(value: JsonElement): String? {
        if (!value.isJsonPrimitive || !value.asJsonPrimitive.isNumber) {
            return null
        }

        return value.asJsonPrimitive.asString
    }

    private fun booleanOf(value: JsonElement?): Boolean? {
        if (value == null || !value.isJsonPrimitive || !value.asJsonPrimitive.isBoolean) {
            return null
        }

        return value.asBoolean
    }

    private fun textOf(value: JsonElement?): String? {
        if (value == null || !value.isJsonPrimitive || !value.asJsonPrimitive.isString) {
            return null
        }

        return value.asString
    }

    private fun codePointOf(value: JsonElement): Char? {
        val code = exactInteger(value, Int.SIZE_BITS)?.toInt() ?: return null

        return if (code in Char.MIN_VALUE.code..Char.MAX_VALUE.code) Char(code) else null
    }

    private fun dataViewOf(value: JsonElement): DataView {
        // A handle is read only from a positive integer that fits, and anything else names no buffer, which is what every reader then refuses.

        val handle =
            if (value.isJsonObject) {
                value.asJsonObject["buffer"]?.let { exactInteger(it, Long.SIZE_BITS)?.toLong() }
            } else {
                null
            }

        if (handle == null || handle < 0L) {
            Log.e("[JsonSerializer : dataViewOf] Value is not a data view")
            return DataView(0, false)
        }

        return DataView(handle, false)
    }

    private fun createGson(): Gson {
        // A parameter without a value has to reach the wire as null, and gson drops it from the object otherwise.
        // Gson escapes markup characters by default, which the wire writes as themselves.
        val builder = GsonBuilder().serializeNulls().disableHtmlEscaping().setStrictness(Strictness.STRICT)

        // Gson asks the adapters registered last first, so registering this one first leaves every type below its own reader and fills any other kotlin class through its constructor.
        builder.registerTypeAdapterFactory(KotlinClassAdapterFactory())

        // A number of no declared type is read as a double by default, which rounds a wide integer and reads a magnitude past the double as infinity.
        builder.setObjectToNumberStrategy(exactNumbers)
        builder.setNumberToNumberStrategy(exactNumbers)

        val customDateSerializer =
            object : JsonDeserializer<Any?>, com.google.gson.JsonSerializer<Date?> {
                override fun deserialize(
                    json: JsonElement,
                    typeOfT: Type?,
                    context: JsonDeserializationContext?
                ): Any =
                    exactInteger(json, Long.SIZE_BITS)?.let { Date(it.toLong()) }
                        ?: throw JsonParseException("The value is not a date")

                override fun serialize(
                    src: Date?,
                    typeOfSrc: Type?,
                    context: JsonSerializationContext?
                ): JsonElement? = src?.let { JsonPrimitive(it.time) }
            }

        builder.registerTypeAdapter(Date::class.java, customDateSerializer)

        val customCharSerializer =
            object : JsonDeserializer<Any?>, com.google.gson.JsonSerializer<Char?> {
                override fun deserialize(
                    json: JsonElement,
                    typeOfT: Type?,
                    context: JsonDeserializationContext?
                ): Any = codePointOf(json) ?: throw JsonParseException("The value is not a character")

                override fun serialize(
                    src: Char?,
                    typeOfSrc: Type?,
                    context: JsonSerializationContext?
                ): JsonElement? = src?.let { JsonPrimitive(it.code) }
            }

        builder.registerTypeAdapter(Char::class.java, customCharSerializer)
        builder.registerTypeAdapter(Char::class.javaObjectType, customCharSerializer)

        // Gson writes a view by reflection, so its one field is the wire format, while reading one goes through a guard that refuses anything but a handle.
        val customDataViewDeserializer = JsonDeserializer<DataView> { json, _, _ -> dataViewOf(json) }

        builder.registerTypeAdapter(DataView::class.java, customDataViewDeserializer)

        registerStrictScalars(builder)

        return builder.create()
    }

    // An integer that fits a long is kept as one and anything else is a double that holds the literal, which is how a value read without a numeric type stays exact.
    // Gson hands a number field a text as well, which is refused like a text read as any other number.
    private val exactNumbers =
        ToNumberStrategy { reader ->
            if (reader.peek() != JsonToken.NUMBER) {
                throw JsonParseException("The value is not a number")
            }

            val literal = reader.nextString()

            exactInteger(literal, Long.SIZE_BITS)?.toLong()
                ?: exactFloating(literal, Double.MAX_VALUE)
                ?: throw JsonParseException("The number is not held exactly")
        }

    // Gson converts a number, a boolean or a text inside a list, a map or a consumer type loosely, so each one is read through the same checks a value at the top is.
    private fun registerStrictScalars(builder: GsonBuilder) {
        val scalars: List<Pair<Class<*>, (JsonElement) -> Any?>> =
            listOf(
                Int::class.javaObjectType to { value -> exactInteger(value, Int.SIZE_BITS)?.toInt() },
                Long::class.javaObjectType to { value -> exactInteger(value, Long.SIZE_BITS)?.toLong() },
                Short::class.javaObjectType to { value -> exactInteger(value, Short.SIZE_BITS)?.toShort() },
                Byte::class.javaObjectType to { value -> exactInteger(value, Byte.SIZE_BITS)?.toByte() },
                Double::class.javaObjectType to { value -> exactFloating(value, Double.MAX_VALUE) },
                Float::class.javaObjectType to { value -> exactFloating(value, Float.MAX_VALUE.toDouble())?.toFloat() },
                Boolean::class.javaObjectType to { value -> booleanOf(value) },
                String::class.java to { value -> textOf(value) },
            )

        for ((type, reader) in scalars) {
            val deserializer =
                JsonDeserializer<Any> { json, _, _ ->
                    reader(json) ?: throw JsonParseException("The value is not a ${type.simpleName}")
                }

            builder.registerTypeAdapter(type, deserializer)

            type.kotlin.javaPrimitiveType?.let { builder.registerTypeAdapter(it, deserializer) }
        }
    }
}
