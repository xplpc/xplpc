package com.xplpc.map

import com.xplpc.message.Message
import com.xplpc.message.Response

class MappingItem(val target: (data: Message, r: Response) -> Unit)
