package com.xplpc.helper

import java.nio.ByteBuffer

object ByteBufferHelper {
    external fun getPtrAddress(data: ByteBuffer): Long
}
