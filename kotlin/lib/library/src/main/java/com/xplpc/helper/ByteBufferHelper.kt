package com.xplpc.helper

import java.nio.ByteBuffer

object ByteBufferHelper {
    external fun getPtrAddress(data: ByteBuffer): Long
    external fun createFromPtr(ptr: Long, size: Int): ByteBuffer
}
