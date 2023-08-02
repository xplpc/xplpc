package com.xplpc.util

import android.util.Log
import com.xplpc.system.Constants

object Log {
    var isEnabled: Boolean = false

    fun d(msg: String) {
        if (isEnabled) {
            Log.d(Constants.LOG_GROUP, msg)
        }
    }

    fun e(msg: String) {
        Log.e(Constants.LOG_GROUP, msg)
    }
}
