package custom

import com.xplpc.message.Message
import com.xplpc.message.Response

object Callback {
    fun batteryLevel(
        data: Message,
        r: Response
    ) {
        val level = 100

        val suffix = data.value<String>("suffix")

        if (suffix != null) {
            r("$level$suffix")
        } else {
            r("$level")
        }
    }
}
