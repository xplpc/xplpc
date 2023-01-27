package com.xplpc.util

import android.util.Log
import com.xplpc.library.BuildConfig
import com.xplpc.system.Constants

object Log {
    val ENABLED: Boolean = BuildConfig.DEBUG

    fun d(msg: String) {
        if (ENABLED) {
            Log.d(Constants.LOG_GROUP, msg)
        }
    }

    fun e(msg: String) {
        Log.e(Constants.LOG_GROUP, msg)
    }
}
