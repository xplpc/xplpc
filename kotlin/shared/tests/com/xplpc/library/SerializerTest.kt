package com.xplpc.library

import com.google.gson.JsonElement
import com.google.gson.JsonParser
import com.google.gson.reflect.TypeToken
import com.xplpc.client.Client
import com.xplpc.message.Message
import com.xplpc.message.Param
import com.xplpc.serializer.JsonSerializer
import com.xplpc.type.DataView
import java.util.Date
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertNotNull
import kotlin.test.assertNull
import kotlin.test.assertTrue

class SerializerTest {
    private val serializer = JsonSerializer()

    private fun element(json: String): JsonElement = JsonParser.parseString(json)

    private inline fun <reified T> read(json: String): T? = serializer.decodeValue(element(json), object : TypeToken<T>() {})

    private fun encodedValue(param: Param): JsonElement {
        val data = JsonSerializer().encodeRequest("sample.wire", param)
        return JsonParser
            .parseString(data)
            .asJsonObject["p"]
            .asJsonArray[0]
            .asJsonObject["v"]
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

    // A view crosses as the handle of a registered buffer and nothing else, so no address ever reaches the wire.
    @Test
    fun dataViewTravelsAsAHandle() {
        val data = JsonSerializer().encodeRequest("sample.wire", Param("dataView", DataView(42, false)))

        assertEquals("{\"f\":\"sample.wire\",\"p\":[{\"n\":\"dataView\",\"v\":{\"buffer\":42}}]}", data)
    }

    @Test
    fun dataViewReadsTheHandleItCarries() {
        assertEquals(42L, read<DataView>("{\"buffer\":42}")?.buffer)
    }

    // A handle is a positive integer, so a boolean, a fraction, a negative number, a string or a number past the width names no buffer at all.
    @Test
    fun dataViewThatIsNotAHandleNamesNoBuffer() {
        val documents =
            listOf(
                "{\"buffer\":true}",
                "{\"buffer\":1.5}",
                "{\"buffer\":-8}",
                "{\"buffer\":\"42\"}",
                "{\"buffer\":18446744073709551615}",
                "{}",
            )

        for (document in documents) {
            assertEquals(0L, read<DataView>(document)?.buffer, document)
        }
    }

    // A kotlin class is filled through its constructor, so a field it declares as never null refuses the value when the wire carries none, and a default it declares fills what the wire left out.
    @Test
    fun aKotlinClassIsFilledThroughItsConstructor() {
        assertEquals(Defaulted("a", 18), read<Defaulted>("{\"name\":\"a\"}"))
        assertEquals(Defaulted("a", 3), read<Defaulted>("{\"name\":\"a\",\"age\":3}"))
        assertNull(read<Defaulted>("{\"name\":null}"))
        assertNull(read<Defaulted>("{\"age\":3}"))
        assertNull(read<Defaulted>("{\"name\":\"a\",\"age\":null}"))

        assertEquals(Noted(null), read<Noted>("{}"))
        assertEquals(Noted(null), read<Noted>("{\"note\":null}"))
        assertEquals(listOf("a", null), read<Listed>("{\"names\":[\"a\",null]}")?.names)
        assertNull(read<Counted>("{\"count\":null}"))
    }

    // A field the class declares in its body is filled after the constructor, through the same checks.
    @Test
    fun aFieldDeclaredInTheBodyIsFilledAfterTheConstructor() {
        assertEquals("b", read<Bodied>("{\"name\":\"a\",\"label\":\"b\"}")?.label)
        assertEquals("none", read<Bodied>("{\"name\":\"a\"}")?.label)
        assertNull(read<Bodied>("{\"name\":\"a\",\"label\":null}"))
    }

    // A field declared through a type parameter is read as the argument the caller asked for, null included.
    @Test
    fun aGenericClassReadsItsFieldAsTheTypeArgument() {
        TestPlatform.initialize()

        assertEquals(Boxed(2), read<Boxed<Int>>("{\"value\":2}"))
        assertNull(read<Boxed<Int>>("{\"value\":2.5}"))
        assertEquals(Boxed(listOf(1, 2)), Client.decode<Boxed<List<Int>>>("{\"r\":{\"value\":[1,2]}}", "test"))
        assertNull(Client.decode<Boxed<List<Int>>>("{\"r\":{\"value\":[1,null]}}", "test"))
        assertEquals(Boxed(listOf(1, null)), Client.decode<Boxed<List<Int?>>>("{\"r\":{\"value\":[1,null]}}", "test"))
    }

    // A class that refuses a value in its own initializer refuses it here too, rather than ending the call.
    @Test
    fun aClassRefusingAValueInItsInitializerIsRefused() {
        assertEquals(Checked(1), read<Checked>("{\"positive\":1}"))
        assertNull(read<Checked>("{\"positive\":-1}"))
    }

    // The jvm erases what a list declares about null, so a value read for a mapping is checked against the type it asks for.
    @Test
    fun aListHoldingNullWhereItsTypeHoldsNoneIsAnotherType() {
        TestPlatform.initialize()

        val message = Message()
        message.set("values", element("[1,null]"))

        assertNull(message.get<List<Int>>("values"))
        assertEquals(listOf(1, null), message.get<List<Int?>>("values"))
    }

    // Anything that is not a view names no buffer, at the top of a value as much as inside one.
    @Test
    fun aValueThatIsNotAnObjectIsTheEmptyView() {
        assertEquals(0L, read<DataView>("42")?.buffer)
        assertEquals(0L, read<DataView>("\"buffer\"")?.buffer)
        assertEquals(0L, read<DataView>("[42]")?.buffer)
    }

    // A nested date or character the wire got wrong refuses the whole value rather than building an object that holds null in a field that cannot be null.
    @Test
    fun aNestedDateOrCharacterThatDoesNotFitRefusesTheWholeValue() {
        assertNotNull(read<Stamped>("{\"createdAt\":494938800,\"initial\":65}"))
        assertNull(read<Stamped>("{\"createdAt\":\"123\",\"initial\":65}"))
        assertNull(read<Stamped>("{\"createdAt\":494938800,\"initial\":1.5}"))
    }

    // A number, a boolean or a text inside a list, a map or a consumer type is read as strictly as one at the top, so gson's own conversions never decide it.
    @Test
    fun aNestedValueIsReadAsExactlyAsOneAtTheTop() {
        assertEquals(listOf(1, 2), read<List<Int>>("[1,2]"))
        assertNull(read<List<Int>>("[4294967297]"))
        assertNull(read<List<Int>>("[2.5]"))
        assertNull(read<List<Long>>("[true]"))
        assertNull(read<Map<String, Long>>("{\"a\":1.5}"))
        assertNull(read<List<Double>>("[\"1\"]"))
        assertNull(read<List<Float>>("[1e300]"))

        assertNotNull(read<Flagged>("{\"done\":true,\"name\":\"x\"}"))
        assertNull(read<Flagged>("{\"done\":\"true\",\"name\":\"x\"}"))
        assertNull(read<Flagged>("{\"done\":true,\"name\":5}"))
    }

    // A number read without a numeric type keeps the integer it carries and is refused past what a double holds, as one read as a declared type is.
    @Test
    fun aNumberOfNoDeclaredTypeIsReadExactly() {
        assertEquals(mapOf("a" to 9_007_199_254_740_993L), read<Map<String, Any>>("{\"a\":9007199254740993}"))
        assertEquals(listOf(2.5), read<List<Any>>("[2.5]"))
        assertNull(read<List<Any>>("[1e400]"))
        assertNull(read<List<Any>>("[18446744073709551617]"))
        assertNull(read<Counted>("{\"count\":1e400}"))
        assertNull(read<Counted>("{\"count\":\"7\"}"))
        assertEquals(7L, read<Counted>("{\"count\":7}")?.count)
    }

    // Encoding a value gson cannot finish, such as one that holds itself, answers the empty value rather than raising out of the serializer.
    @Test
    fun anAnswerThatCannotBeWrittenIsTheEmptyValue() {
        val circular = mutableListOf<Any>()
        circular.add(circular)

        assertEquals("", JsonSerializer().encodeFunctionReturnValue(circular))
    }

    // A nested value that is not a view names no buffer, which is the empty view every other bridge answers.
    @Test
    fun aNestedValueThatIsNotAViewIsTheEmptyView() {
        assertEquals(0L, read<Viewed>("{\"view\":5}")?.view?.buffer)
    }

    @Test
    fun longKeepsItsPrecision() {
        val value = encodedValue(Param("int64", 9_007_199_254_740_993L))

        assertEquals(9_007_199_254_740_993L, value.asLong)
    }

    @Test
    fun nullReturnValueIsPreserved() {
        // A mapping with nothing to answer travels as a null, the same as every other bridge.

        val data = JsonSerializer().encodeFunctionReturnValue(null)

        assertTrue(JsonParser.parseString(data).asJsonObject["r"].isJsonNull)
    }

    @Test
    fun aNumberThatIsNotFiniteIsRefused() {
        // The wire has no token for infinity, so writing one would put a document on it that no other bridge can read.

        val serializer = JsonSerializer()

        assertEquals("", serializer.encodeFunctionReturnValue(Double.POSITIVE_INFINITY))
        assertEquals("", serializer.encodeFunctionReturnValue(Double.NaN))
        assertEquals("{\"r\":2.5}", serializer.encodeFunctionReturnValue(2.5))
    }

    @Test
    fun aByteAndAShortAreReadOnlyWhenTheyHoldTheValue() {
        assertEquals(Byte.MIN_VALUE, read<Byte>("-128"))
        assertNull(read<Byte>("128"))
        assertEquals(Short.MAX_VALUE, read<Short>("32767"))
        assertNull(read<Short>("32768"))
    }

    @Test
    fun aFractionIsNeverReadAsAnInteger() {
        assertNull(read<Int>("2.0"))
        assertNull(read<Long>("1e3"))
    }

    // A magnitude past the type would arrive as infinity and an integer past its precision would arrive as another integer.
    @Test
    fun aFloatingValueIsReadOnlyWhenTheTypeHoldsIt() {
        assertNull(read<Float>("-1e300"))
        assertNull(read<Float>("16777217"))
        assertNull(read<Double>("1e1000"))
        assertNull(read<Double>("9007199254740993"))
        assertNull(read<Double>("18446744073709551615"))

        assertEquals(Float.MAX_VALUE, read<Float>("3.4028234663852886e38"))
        assertEquals(16_777_216f, read<Float>("16777216"))
        assertEquals(2.5f, read<Float>("2.5"))
        assertEquals(1e300, read<Double>("1e300"))
        assertEquals(9_007_199_254_740_992.0, read<Double>("9007199254740992"))
        assertEquals(-9_223_372_036_854_775_808.0, read<Double>("-9223372036854775808"))
    }

    @Test
    fun aCharacterIsACodePointThatFitsOne() {
        assertEquals('z', read<Char>("122"))
        assertNull(read<Char>("-1"))
        assertNull(read<Char>("65536"))
        assertNull(read<Char>("\"z\""))
    }

    @Test
    fun aFloatingValueWithAnExponentPastWhatADecimalHoldsIsReadRatherThanRaising() {
        assertEquals(0.0, read<Double>("0e9999999999"))
        assertEquals(0.0f, read<Float>("1e-9999999999"))
        assertNull(read<Double>("1e9999999999"))
    }

    @Test
    fun aDateIsReadOnlyFromWholeMillisecondsThatFit() {
        assertEquals(Date(494_938_800), read<Date>("494938800"))
        assertNull(read<Date>("1.5"))
        assertNull(read<Date>("1e30"))
        assertNull(read<Date>("\"494938800\""))
        assertNull(read<Date>("true"))
    }

    // A character that does not fit refuses the whole list, the way the reference refuses a list carrying one number its type cannot hold.
    @Test
    fun aCharacterInsideAListIsReadOnlyFromACodePointThatFitsOne() {
        assertEquals(listOf('z'), read<List<Char>>("[122]"))
        assertNull(read<List<Char?>>("[65.9]"))
        assertNull(read<List<Char?>>("[\"65\"]"))
    }

    @Test
    fun aValueSetFromKotlinIsOnlyCheckedAgainstTheType() {
        assertEquals(10, serializer.decodeValue(10, object : TypeToken<Int>() {}))
        assertNull(serializer.decodeValue(10, object : TypeToken<Long>() {}))
        assertNull(serializer.decodeValue(10, object : TypeToken<String>() {}))
    }
}

data class Stamped(
    val createdAt: Date,
    val initial: Char
)

data class Viewed(
    val view: DataView
)

data class Flagged(
    val done: Boolean,
    val name: String
)

data class Counted(
    val count: Number
)

data class Defaulted(
    val name: String,
    val age: Int = 18
)

data class Noted(
    val note: String?
)

data class Listed(
    val names: List<String?>
)

data class Boxed<T>(
    val value: T
)

data class Checked(
    val positive: Int
) {
    init {
        require(positive > 0)
    }
}

class Bodied(
    val name: String
) {
    var label: String = "none"
}
