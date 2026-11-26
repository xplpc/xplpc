package com.xplpc.message

import com.xplpc.core.XPLPC

class Request(
    functionName: String,
    vararg params: Param
) {
    val data: String = XPLPC.config.serializer.encodeRequest(functionName, *params)
}
