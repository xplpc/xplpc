package com.xplpc.library

import com.google.gson.JsonParser
import com.google.gson.reflect.TypeToken
import com.xplpc.message.Param
import com.xplpc.serializer.JsonSerializer
import com.xplpc.type.DataView
import java.util.Date
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertNull
import kotlin.test.assertTrue

class SerializerTest {
    private fun encodedValue(param: Param): com.google.gson.JsonElement {
        val data = JsonSerializer().encodeRequest("sample.wire", param)
        return JsonParser.parseString(data).asJsonObject["p"].asJsonArray[0].asJsonObject["v"]
    }

    // The reference format is what the c++ core writes, and every bridge has to agree with it.
    @Test
    fun charTravelsAsACodePointNumber() {
        val value = encodedValue(Param("char", 'z'))

        assertTrue(value.asJsonPrimitive.isNumber)
        assertEquals(122, value.asInt)
    }

    @Test
    fun dateTravelsAsWholeMillisecondsSinceEpoch() {
        val value = encodedValue(Param("date", Date(494_938_800)))

        assertTrue(value.asJsonPrimitive.isNumber)
        assertEquals(494_938_800, value.asLong)
    }

    @Test
    fun dataViewTravelsAsPointerAndSize() {
        val value = encodedValue(Param("dataView", DataView(128, 4))).asJsonObject

        assertEquals(128, value["ptr"].asLong)
        assertEquals(4, value["size"].asInt)
    }

    @Test
    fun longKeepsItsPrecision() {
        val value = encodedValue(Param("int64", 9_007_199_254_740_993L))

        assertEquals(9_007_199_254_740_993L, value.asLong)
    }

    @Test
    fun nullValueIsPreserved() {
        val data = JsonSerializer().encodeRequest("sample.wire", Param("value", null))
        val param = JsonParser.parseString(data).asJsonObject["p"].asJsonArray[0].asJsonObject

        assertEquals("value", param["n"].asString)
        assertTrue(param["v"].isJsonNull)
    }

    @Test
    fun nullReturnValueIsPreserved() {
        // A mapping with nothing to answer travels as a null, the same as every other bridge.

        val data = JsonSerializer().encodeFunctionReturnValue(null)

        assertTrue(JsonParser.parseString(data).asJsonObject["r"].isJsonNull)
    }

    // The reference format is what the c++ core writes, and a serializer that spaces its output carries different bytes for the same value.
    @Test
    fun theWireCarriesNoInsignificantWhitespace() {
        val data = JsonSerializer().encodeRequest("sample.wire", Param("a", 1), Param("b", "x"))

        assertEquals("{\"f\":\"sample.wire\",\"p\":[{\"n\":\"a\",\"v\":1},{\"n\":\"b\",\"v\":\"x\"}]}", data)
    }

    // A request with no parameters carries an empty array, never null and never absent.
    @Test
    fun requestWithoutParamsCarriesAnEmptyArray() {
        val data = JsonSerializer().encodeRequest("sample.ping")
        val params = JsonParser.parseString(data).asJsonObject["p"]

        assertTrue(params.isJsonArray)
        assertEquals(0, params.asJsonArray.size())
    }

    // Every bridge answers the empty value for its type when the data cannot be read.
    @Test
    fun invalidDataAnswersEmpty() {
        val serializer = JsonSerializer()

        assertNull(serializer.decodeRequest("not-a-json"))
        assertEquals("", serializer.decodeRequest("{}")?.functionName)
        assertNull(serializer.decodeFunctionReturnValue("not-a-json", object : TypeToken<String>() {}))
        assertNull(serializer.decodeFunctionReturnValue("{\"r\":null}", object : TypeToken<String>() {}))

        // The empty string is what every failing path answers with, so it is the documented answer rather than a document that failed to parse.
        assertNull(serializer.decodeFunctionReturnValue("", object : TypeToken<String>() {}))
    }
    @Test
    fun aNumberThatIsNotFiniteIsRefused() {
        // The wire has no token for infinity, so writing one would put a document on it that no other bridge can read.

        val serializer = JsonSerializer()

        assertEquals("", serializer.encodeFunctionReturnValue(Double.POSITIVE_INFINITY))
        assertEquals("", serializer.encodeFunctionReturnValue(Double.NaN))
        assertEquals("{\"r\":2.5}", serializer.encodeFunctionReturnValue(2.5))
    }
}
