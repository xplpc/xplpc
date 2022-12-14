package com.xplpc.map

import com.xplpc.message.Message
import com.xplpc.message.Response

class MappingItem(val name: String, val target: (data: Message) -> Response)
