package com.xplpc.util

import com.xplpc.system.Constants
import java.util.logging.Level
import java.util.logging.Logger

object Log {
    private val logger: Logger = Logger.getLogger(Constants.LOG_GROUP)

    @Volatile
    var isEnabled: Boolean = false
        set(value) {
            field = value

            // The logger filters before any handler sees the record, so raising it is what lets a debug line reach one at all.
            logger.level = if (value) Level.FINE else Level.INFO
        }

    fun d(msg: String) {
        if (isEnabled) {
            logger.fine(msg)
        }
    }

    fun e(msg: String) {
        logger.severe(msg)
    }
}
