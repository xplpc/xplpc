package com.xplpc.library

import androidx.test.ext.junit.runners.AndroidJUnit4
import com.xplpc.client.RemoteClient
import com.xplpc.core.Config
import com.xplpc.core.XPLPC
import com.xplpc.message.Param
import com.xplpc.message.Request
import com.xplpc.serializer.JsonSerializer
import kotlinx.coroutines.runBlocking
import org.junit.Assert.assertEquals
import org.junit.Assert.assertNotNull
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith
import java.util.Date

@RunWith(AndroidJUnit4::class)
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

    private fun createItem(): AllTypes {
        return AllTypes(
            1,
            2,
            3,
            4,
            1.1f,
            2.2,
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

    @Before
    fun setUp() {
        XPLPC.initialize(
            Config(JsonSerializer()),
        )
    }

    @Test
    fun singleItem() {
        val item = createItem()
        item.typeList.add(createItem())
        item.typeMap["item1"] = "ok"

        val request = Request("sample.alltypes.single", Param("item", item))
        println(request.data)
        val r = RemoteClient.call<AllTypes>(request)

        assertNotNull(r)

        r?.let { allTypes ->
            val mapValue = allTypes.typeMap["item1"]

            assertEquals(1.toByte(), allTypes.typeInt8)
            assertEquals(2.toShort(), allTypes.typeInt16)
            assertEquals(3, allTypes.typeInt32)
            assertEquals(4, allTypes.typeInt64)
            assertEquals(1.1f, allTypes.typeFloat32)
            assertEquals(2.2, allTypes.typeFloat64, 0.0)
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

    @Test
    fun singleItemAsync() {
        val item = createItem()
        item.typeList.add(createItem())
        item.typeMap["item1"] = "ok"

        val request = Request("sample.alltypes.single", Param("item", item))

        runBlocking {
            val r = RemoteClient.callAsync<AllTypes>(request).await()

            assertNotNull(r)

            r?.let { allTypes ->
                val mapValue = allTypes.typeMap["item1"]

                assertEquals(1.toByte(), allTypes.typeInt8)
                assertEquals(2.toShort(), allTypes.typeInt16)
                assertEquals(3, allTypes.typeInt32)
                assertEquals(4, allTypes.typeInt64)
                assertEquals(1.1f, allTypes.typeFloat32)
                assertEquals(2.2, allTypes.typeFloat64, 0.0)
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
    fun multipleItems() {
        val item1 = createItem()
        item1.typeList.add(createItem())

        val item2 = createItem()
        item2.typeList.add(createItem())

        val request = Request("sample.alltypes.list", Param("items", listOf(item1, item2)))
        val r = RemoteClient.call<List<AllTypes>>(request)

        assertEquals(2, r?.size)
    }

    @Test
    fun multipleItemsAsync() {
        val item1 = createItem()
        item1.typeList.add(createItem())

        val item2 = createItem()
        item2.typeList.add(createItem())

        val request = Request("sample.alltypes.list", Param("items", listOf(item1, item2)))

        runBlocking {
            val r = RemoteClient.callAsync<List<AllTypes>>(request).await()
            assertEquals(2, r?.size)
        }
    }
}
