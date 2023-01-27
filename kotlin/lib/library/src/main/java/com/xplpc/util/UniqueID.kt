package com.xplpc.util

import java.util.concurrent.atomic.AtomicLong

object UniqueID {
    private val uid = AtomicLong()

    fun generate(): String {
        return uid.addAndGet(1).toString()
    }
}
