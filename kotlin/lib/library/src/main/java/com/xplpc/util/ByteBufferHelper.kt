package com.xplpc.util

import java.nio.ByteBuffer

object ByteBufferHelper {
    external fun getByteBufferAddress(array: ByteBuffer): Long
    external fun getByteBufferFromAddress(address: Long, size: Int): ByteBuffer
}
