package com.xplpc.library

import com.xplpc.client.Client
import com.xplpc.message.Param
import com.xplpc.message.Request
import kotlinx.coroutines.runBlocking
import java.util.Date
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertTrue
import kotlin.test.assertNotNull

class AllTypesTest {
    data class AllTypes(
        var typeInt8: Byte,
        var typeInt16: Short,
        var typeInt32: Int,
        var typeInt64: Long,
        var typeFloat32: Float,
        var typeFloat64: Double,
        var typeBool: Boolean,
        var typeOptional: Boolean?,
        var typeList: ArrayList<AllTypes>,
        var typeMap: HashMap<String, String>,
        var typeDateTime: Date,
        var typeChar: Char,
        var typeChar16: Char,
        var typeChar32: Char,
        var typeWchar: Char,
        var typeString: String
    )

    // The jni boundary clears anything the callback raises, so a failed check is captured and raised here.
    @Suppress("TooGenericExceptionCaught")
    private inline fun <reified T> verifyCall(
        request: Request,
        crossinline block: (T?) -> Unit
    ) {
        var failure: Throwable? = null
        var answered = false

        Client.call<T>(request) { response ->
            answered = true

            try {
                block(response)
            } catch (e: Throwable) {
                failure = e
            }
        }

        assertTrue(answered, "the callback was never invoked")
        failure?.let { throw it }
    }

    private fun setUp() {
        TestPlatform.initialize()
    }

    private fun createItem(): AllTypes {
        // The edges are what a serializer loses, since a small value round trips even when the format cannot carry the type.
        return AllTypes(
            Byte.MIN_VALUE,
            Short.MIN_VALUE,
            Int.MIN_VALUE,
            9_007_199_254_740_993L,
            0.1f,
            0.1,
            true,
            null,
            ArrayList(),
            HashMap(),
            Date(494_938_800),
            'z',
            '®',
            '®',
            '®',
            "ok",
        )
    }

    @Test
    fun singleItem() {
        setUp()

        val item = createItem()
        item.typeList.add(createItem())
        item.typeMap["item1"] = "ok"

        val request = Request("sample.alltypes.single", Param("item", item))

        verifyCall<AllTypes>(request) { response ->
            assertNotNull(response)

            response.let { allTypes ->
                val mapValue = allTypes.typeMap["item1"]

                assertEquals(Byte.MIN_VALUE, allTypes.typeInt8)
                assertEquals(Short.MIN_VALUE, allTypes.typeInt16)
                assertEquals(Int.MIN_VALUE, allTypes.typeInt32)
                assertEquals(9_007_199_254_740_993L, allTypes.typeInt64)
                assertEquals(0.1f, allTypes.typeFloat32)
                assertEquals(0.1, allTypes.typeFloat64, 0.0)
                assertEquals(true, allTypes.typeBool)
                assertEquals(null, allTypes.typeOptional)
                assertEquals(1, allTypes.typeList.size)
                assertEquals("ok", mapValue)
                assertEquals(494_938_800, allTypes.typeDateTime.time)
                assertEquals('z', allTypes.typeChar)
                assertEquals('®', allTypes.typeChar16)
                assertEquals('®', allTypes.typeChar32)
                assertEquals('®', allTypes.typeWchar)
                assertEquals("ok", allTypes.typeString)
            }
        }
    }

    @Test
    fun singleItemAsync() {
        setUp()

        val item = createItem()
        item.typeList.add(createItem())
        item.typeMap["item1"] = "ok"

        val request = Request("sample.alltypes.single", Param("item", item))

        runBlocking {
            verifyCall<AllTypes>(request) { response ->
                assertNotNull(response)

                response.let { allTypes ->
                    val mapValue = allTypes.typeMap["item1"]

                    assertEquals(Byte.MIN_VALUE, allTypes.typeInt8)
                    assertEquals(Short.MIN_VALUE, allTypes.typeInt16)
                    assertEquals(Int.MIN_VALUE, allTypes.typeInt32)
                    assertEquals(9_007_199_254_740_993L, allTypes.typeInt64)
                    assertEquals(0.1f, allTypes.typeFloat32)
                    assertEquals(0.1, allTypes.typeFloat64, 0.0)
                    assertEquals(true, allTypes.typeBool)
                    assertEquals(null, allTypes.typeOptional)
                    assertEquals(1, allTypes.typeList.size)
                    assertEquals("ok", mapValue)
                    assertEquals(494_938_800, allTypes.typeDateTime.time)
                    assertEquals('z', allTypes.typeChar)
                    assertEquals('®', allTypes.typeChar16)
                    assertEquals('®', allTypes.typeChar32)
                    assertEquals('®', allTypes.typeWchar)
                    assertEquals("ok", allTypes.typeString)
                }
            }
        }
    }

    @Test
    fun multipleItems() {
        setUp()

        val item1 = createItem()
        item1.typeList.add(createItem())

        val item2 = createItem()
        item2.typeList.add(createItem())

        val request = Request("sample.alltypes.list", Param("items", listOf(item1, item2)))

        verifyCall<List<AllTypes>>(request) { response ->
            assertEquals(2, response?.size)
        }
    }

    @Test
    fun multipleItemsAsync() {
        setUp()

        val item1 = createItem()
        item1.typeList.add(createItem())

        val item2 = createItem()
        item2.typeList.add(createItem())

        val request = Request("sample.alltypes.list", Param("items", listOf(item1, item2)))

        runBlocking {
            verifyCall<List<AllTypes>>(request) { response ->
                assertEquals(2, response?.size)
            }
        }
    }
}
