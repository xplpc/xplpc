package com.xplpc.library

import com.xplpc.client.Client
import com.xplpc.message.Request
import kotlin.test.assertEquals

// The jni boundary clears anything the callback raises, so a failed check is captured and raised here, and a callback that never ran or ran twice fails the test.
inline fun <reified T> verifyCall(
    request: Request,
    crossinline block: (T?) -> Unit
) {
    var failure: Throwable? = null
    var answers = 0

    Client.call<T>(request) { response ->
        answers += 1

        try {
            block(response)
        } catch (e: Throwable) {
            failure = e
        }
    }

    assertEquals(1, answers, "the callback has to run exactly once")
    failure?.let { throw it }
}

fun verifyCall(
    requestData: String,
    block: (String) -> Unit
) {
    var failure: Throwable? = null
    var answers = 0

    Client.call(requestData) { response ->
        answers += 1

        try {
            block(response)
        } catch (e: Throwable) {
            failure = e
        }
    }

    assertEquals(1, answers, "the callback has to run exactly once")
    failure?.let { throw it }
}
