package com.xplpc.runner.permission

import android.app.Activity
import android.content.pm.PackageManager
import androidx.core.app.ActivityCompat

class PermissionHandler(
    private val activity: Activity,
    private val permissions: Array<String>
) {
    init {
        if (!hasPermissions()) {
            ActivityCompat.requestPermissions(activity, permissions, PERMISSION_ALL)
        }
    }

    fun hasPermissions(): Boolean =
        permissions.all {
            ActivityCompat.checkSelfPermission(activity, it) == PackageManager.PERMISSION_GRANTED
        }

    private companion object {
        const val PERMISSION_ALL = 1
    }
}
