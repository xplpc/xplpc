package com.xplpc.runner.app

import android.os.StrictMode
import androidx.multidex.MultiDexApplication
import com.xplpc.core.Config
import com.xplpc.core.XPLPC
import com.xplpc.runner.BuildConfig
import com.xplpc.runner.custom.Mapping
import com.xplpc.serializer.JsonSerializer
import com.xplpc.util.Log

class Application : MultiDexApplication() {
    override fun onCreate() {
        super.onCreate()

        Log.d("[Application : onCreate]")

        instance = this

        initializeStrictMode()
        initializeXPLPC()
    }

    private fun initializeXPLPC() {
        Log.d("[Application : initializeXPLPC]")

        // initialize xplpc
        XPLPC.initialize(
            Config(JsonSerializer()),
        )

        // initialize local mappings
        Mapping.initialize()
    }

    override fun onTerminate() {
        Log.d("[Application : onTerminate] App terminated")
        super.onTerminate()
    }

    private fun initializeStrictMode() {
        Log.d("[Application : initializeStrictMode] Initializing strict mode...")

        if (BuildConfig.DEBUG) {
            StrictMode.setThreadPolicy(
                StrictMode.ThreadPolicy.Builder()
                    .detectAll()
                    .penaltyLog()
                    .build()
            )

            StrictMode.setVmPolicy(
                StrictMode.VmPolicy.Builder()
                    .detectLeakedSqlLiteObjects()
                    .detectLeakedClosableObjects()
                    .penaltyLog()
                    .build()
            )
        }
    }

    companion object {
        lateinit var instance: Application
            private set
    }
}
