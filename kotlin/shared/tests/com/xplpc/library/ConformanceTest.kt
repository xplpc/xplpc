package com.xplpc.library

import com.google.gson.Gson
import com.google.gson.JsonElement
import com.google.gson.JsonObject
import com.google.gson.JsonParser
import com.google.gson.JsonPrimitive
import com.google.gson.reflect.TypeToken
import com.xplpc.client.Client
import com.xplpc.core.XPLPC
import com.xplpc.message.Message
import com.xplpc.message.Param
import com.xplpc.message.Request
import com.xplpc.type.DataView
import org.junit.Assume.assumeTrue
import org.junit.Test
import org.junit.runner.RunWith
import org.junit.runners.Parameterized
import java.util.Date
import kotlin.test.assertEquals
import kotlin.test.assertNotNull
import kotlin.test.assertNull
import kotlin.test.fail

// The cases of the shared conformance suite, one test per file, so a case added to the repository is run by both runtimes without code.
// A parameterized class is the one per case report both runtimes understand, which is why these name the junit annotation the other tests reach through kotlin.test.

private fun readCase(
    group: String,
    name: String
): JsonObject = JsonParser.parseString(ConformanceFiles.read(group, name)).asJsonObject

@RunWith(Parameterized::class)
class WireConformanceTest(
    private val name: String
) {
    companion object {
        @JvmStatic
        @Parameterized.Parameters(name = "{0}")
        fun cases(): List<String> = ConformanceFiles.names("wire")

        private val notSent = mapOf("uint64" to "kotlin declares no unsigned type the serializer writes")

        private val writers: Map<String, (JsonElement) -> Any?> =
            mapOf(
                "int" to { value -> value.asLong },
                "int64" to { value -> value.asLong },
                "double" to { value -> value.asDouble },
                "string" to { value -> value.asString },
                "bool" to { value -> value.asBoolean },
                "null" to { _ -> null },
                "char" to { value -> value.asInt.toChar() },
                "date" to { value -> Date(value.asLong) },
                "dataview" to { value -> DataView(value.asLong, false) },
                "list<int>" to { value -> value.asJsonArray.map { it.asLong } },
                "list<string>" to { value -> value.asJsonArray.map { it.asString } },
                "list<list<int>>" to { value -> value.asJsonArray.map { inner -> inner.asJsonArray.map { it.asLong } } },
                "list<dataview>" to { value -> value.asJsonArray.map { DataView(it.asLong, false) } },
                "map<int>" to { value -> value.asJsonObject.entrySet().associate { it.key to it.value.asLong } },
                "record" to { value -> Recorded(value.asJsonObject["count"].asLong, value.asJsonObject["name"].asString) },
            )
    }

    private fun paramOf(param: JsonObject): Param {
        val type = param["type"].asString
        val write = writers[type] ?: fail("The case declares a parameter type this runner does not know: $type")

        return Param(param["name"].asString, write(param["value"]))
    }

    @Test
    fun encodesTheExactBytes() {
        TestPlatform.initialize()

        val case = readCase("wire", name)
        val unsent = case["params"].asJsonArray.map { it.asJsonObject["type"].asString }.firstOrNull { it in notSent }

        assumeTrue(notSent[unsent] ?: "", unsent == null)

        val params = case["params"].asJsonArray.map { paramOf(it.asJsonObject) }
        val request = Request(case["function"].asString, *params.toTypedArray())

        assertEquals(case["bytes"].asString, request.data)
    }
}

@RunWith(Parameterized::class)
class AcceptConformanceTest(
    private val name: String
) {
    companion object {
        @JvmStatic
        @Parameterized.Parameters(name = "{0}")
        fun cases(): List<String> = ConformanceFiles.names("accept")
    }

    @Test
    fun readsTheFunctionAndItsParameters() {
        TestPlatform.initialize()

        val case = readCase("accept", name)
        val request = XPLPC.config.serializer.decodeRequest(case["document"].asString)

        assertNotNull(request, "the decoder refused a document every bridge reads")
        assertEquals(case["function"].asString, request.functionName)

        for ((param, expected) in case["params"].asJsonObject.entrySet()) {
            assertParam(param, expected, request.message)
        }
    }

    private fun assertParam(
        param: String,
        expected: JsonElement,
        message: Message
    ) {
        if (expected.isJsonNull) {
            assertNull(message.get<Any>(param), "parameter \"$param\"")
            return
        }

        val primitive = expected.asJsonPrimitive

        when {
            primitive.isString -> assertEquals(primitive.asString, message.get<String>(param), "parameter \"$param\"")
            primitive.isBoolean -> assertEquals(primitive.asBoolean, message.get<Boolean>(param), "parameter \"$param\"")
            else -> assertEquals(primitive.asLong, message.get<Long>(param), "parameter \"$param\"")
        }
    }
}

@RunWith(Parameterized::class)
class RefuseConformanceTest(
    private val name: String
) {
    companion object {
        @JvmStatic
        @Parameterized.Parameters(name = "{0}")
        fun cases(): List<String> = ConformanceFiles.names("refuse")
    }

    @Test
    fun answersNothing() {
        TestPlatform.initialize()

        val case = readCase("refuse", name)
        val request = XPLPC.config.serializer.decodeRequest(case["document"].asString)

        assertNull(request, "the decoder read a document every bridge refuses, naming function \"${request?.functionName}\"")
    }
}

private val plain = Gson()

// A value read is compared as what it writes back, which is the form the case expects it in, except where that form is not the wire's.
private class ValueReader(
    val read: (String) -> Any?,
    val expected: (JsonElement) -> JsonElement,
    val written: (Any) -> JsonElement
)

// A value is read the way a caller reads an answer, which is where kotlin checks what the declared type says about null.
private inline fun <reified T> readerOf(
    noinline expected: (JsonElement) -> JsonElement = { it },
    noinline written: (Any) -> JsonElement = { plain.toJsonTree(it) }
) = ValueReader({ Client.decode<T>(it, "values") }, expected, written)

@RunWith(Parameterized::class)
class ValuesConformanceTest(
    private val name: String
) {
    companion object {
        @JvmStatic
        @Parameterized.Parameters(name = "{0}")
        fun cases(): List<String> = ConformanceFiles.names("values")

        private val readers: Map<String, ValueReader> =
            mapOf(
                "bool" to readerOf<Boolean>(),
                "int8" to readerOf<Byte>(),
                "int16" to readerOf<Short>(),
                "int32" to readerOf<Int>(),
                "int64" to readerOf<Long>(),
                "float" to
                    readerOf<Float>(
                        expected = { JsonPrimitive(it.asFloat) },
                        written = { JsonPrimitive(it as Float) },
                    ),
                "double" to readerOf<Double>(),
                "string" to readerOf<String>(),
                "char" to readerOf<Char>(written = { JsonPrimitive((it as Char).code) }),
                "date" to readerOf<Date>(written = { JsonPrimitive((it as Date).time) }),
                "dataview" to readerOf<DataView>(written = { JsonPrimitive((it as DataView).buffer) }),
                "list<int32>" to readerOf<List<Int>>(),
                "list<string>" to readerOf<List<String>>(),
                "list<list<int32>>" to readerOf<List<List<Int>>>(),
                "map<int32>" to readerOf<Map<String, Int>>(),
                "todo" to readerOf<TodoTest.Todo>(),
            )

        private const val NO_UNSIGNED = "kotlin declares no unsigned type the serializer reads"

        private val notCarried = mapOf("uint8" to NO_UNSIGNED, "uint16" to NO_UNSIGNED, "uint32" to NO_UNSIGNED, "uint64" to NO_UNSIGNED)
    }

    @Test
    fun readsTheDeclaredType() {
        TestPlatform.initialize()

        val case = readCase("values", name)
        val type = case["type"].asString

        assumeTrue(notCarried[type] ?: "", type !in notCarried)

        val reader = readers[type] ?: fail("The case declares a type this runner does not know: $type")
        val value = reader.read(case["document"].asString)
        val expected = case["expect"]

        if (expected.isJsonNull) {
            assertNull(value)
            return
        }

        assertNotNull(value)
        assertEquals(reader.expected(expected), reader.written(value))
    }
}

// A record of the caller's own whose fields are written in the order every encoder agrees on.
data class Recorded(
    val count: Long,
    val name: String
)
