package com.xplpc.library

import com.xplpc.client.Client
import com.xplpc.message.Param
import com.xplpc.message.Request
import kotlinx.coroutines.runBlocking
import kotlin.test.Test
import kotlin.test.assertEquals

class TodoTest : PendingGuard() {
    data class Todo(
        val id: Long,
        val title: String,
        val body: String,
        val data: Map<String, String>,
        val done: Boolean,
    )

    private fun setUp() {
        TestPlatform.initialize()
    }

    @Test
    fun singleItem() {
        setUp()

        val todo =
            Todo(
                1,
                "Title 1",
                "Body 1",
                mapOf("data1" to "value1", "data2" to "value2"),
                true
            )

        val request = Request("sample.todo.single", Param("item", todo))

        verifyCall<Todo>(request) { response ->
            assertEquals("Title 1", response?.title)
        }
    }

    @Test
    fun singleItemAsync() {
        setUp()

        val todo =
            Todo(
                1,
                "Title 1",
                "Body 1",
                mapOf("data1" to "value1", "data2" to "value2"),
                true
            )

        val request = Request("sample.todo.single", Param("item", todo))

        runBlocking {
            assertEquals("Title 1", Client.callAsync<Todo>(request)?.title)
        }
    }

    @Test
    fun multipleItems() {
        setUp()

        val todo1 =
            Todo(
                1,
                "Title 1",
                "Body 1",
                mapOf("data1" to "value1", "data2" to "value2"),
                true
            )

        val todo2 =
            Todo(
                2,
                "Title 2",
                "Body 2",
                mapOf("data1" to "value1", "data2" to "value2"),
                true
            )

        val request = Request("sample.todo.list", Param("items", listOf(todo1, todo2)))

        verifyCall<List<Todo>>(request) { response ->
            assertEquals(2, response?.size)
            assertEquals("Title 1", response?.get(0)?.title)
            assertEquals("Title 2", response?.get(1)?.title)
        }
    }

    @Test
    fun multipleItemsAsync() {
        setUp()

        val todo1 =
            Todo(
                1,
                "Title 1",
                "Body 1",
                mapOf("data1" to "value1", "data2" to "value2"),
                true
            )

        val todo2 =
            Todo(
                2,
                "Title 2",
                "Body 2",
                mapOf("data1" to "value1", "data2" to "value2"),
                true
            )

        val request = Request("sample.todo.list", Param("items", listOf(todo1, todo2)))

        runBlocking {
            val response = Client.callAsync<List<Todo>>(request)

            assertEquals(2, response?.size)
            assertEquals("Title 1", response?.get(0)?.title)
            assertEquals("Title 2", response?.get(1)?.title)
        }
    }
}
