package com.xplpc.message

import com.xplpc.core.XPLPC

class Request(functionName: String, vararg params: Param) {
    var data: String
        private set

    init {
        data = XPLPC.config.serializer.encodeRequest(functionName, *params)
    }
}
