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
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith

@RunWith(AndroidJUnit4::class)
class TodoTest {
    data class Todo(
        val id: Long,
        val title: String,
        val body: String,
        val data: Map<String, String>,
        val done: Boolean,
    )

    @Before
    fun setUp() {
        XPLPC.initialize(
            Config(JsonSerializer()),
        )
    }

    @Test
    fun singleItem() {
        val todo = Todo(
            1,
            "Title 1",
            "Body 1",
            mapOf("data1" to "value1", "data2" to "value2"),
            true
        )

        val request = Request("sample.todo.single", Param("item", todo))
        val r = RemoteClient.call<Todo>(request)

        assertEquals("Title 1", r?.title)
    }

    @Test
    fun singleItemAsync() {
        val todo = Todo(
            1,
            "Title 1",
            "Body 1",
            mapOf("data1" to "value1", "data2" to "value2"),
            true
        )

        val request = Request("sample.todo.single", Param("item", todo))

        runBlocking {
            val r = RemoteClient.callAsync<Todo>(request).await()
            assertEquals("Title 1", r?.title)
        }
    }

    @Test
    fun multipleItems() {
        val todo1 = Todo(
            1,
            "Title 1",
            "Body 1",
            mapOf("data1" to "value1", "data2" to "value2"),
            true
        )

        val todo2 = Todo(
            2,
            "Title 2",
            "Body 2",
            mapOf("data1" to "value1", "data2" to "value2"),
            true
        )

        val request = Request("sample.todo.list", Param("items", listOf(todo1, todo2)))
        val r = RemoteClient.call<List<Todo>>(request)

        assertEquals(2, r?.size)
        assertEquals("Title 1", r?.get(0)?.title)
        assertEquals("Title 2", r?.get(1)?.title)
    }

    @Test
    fun multipleItemsAsync() {
        val todo1 = Todo(
            1,
            "Title 1",
            "Body 1",
            mapOf("data1" to "value1", "data2" to "value2"),
            true
        )

        val todo2 = Todo(
            2,
            "Title 2",
            "Body 2",
            mapOf("data1" to "value1", "data2" to "value2"),
            true
        )

        val request = Request("sample.todo.list", Param("items", listOf(todo1, todo2)))

        runBlocking {
            val r = RemoteClient.callAsync<List<Todo>>(request).await()

            assertEquals(2, r?.size)
            assertEquals("Title 1", r?.get(0)?.title)
            assertEquals("Title 2", r?.get(1)?.title)
        }
    }
}
