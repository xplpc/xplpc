package com.xplpc.util

import java.util.logging.Logger

object Log {
    var isEnabled: Boolean = false

    fun d(msg: String) {
        if (isEnabled) {
            Logger.getGlobal().fine(msg)
        }
    }

    fun e(msg: String) {
        Logger.getGlobal().severe(msg)
    }
}
