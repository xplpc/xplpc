package com.xplpc.data

@PublishedApi
internal class PendingCall(
    private val callback: (String) -> Unit
) {
    // Called by the native side once, with the answer or with the empty value when the call could not be served.
    fun onAnswer(response: String) {
        callback(response)
    }
}
