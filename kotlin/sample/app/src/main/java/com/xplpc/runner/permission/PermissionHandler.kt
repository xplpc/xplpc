package com.xplpc.runner.permission

import android.app.Activity
import android.content.Context
import android.content.pm.PackageManager
import android.os.Build
import androidx.core.app.ActivityCompat

class PermissionHandler(context: Context?, permissions: Array<String>?) {
    private val context: Context?
    private val permissions: Array<String>?

    init {
        this.context = context
        this.permissions = permissions

        val permissionAll = 1

        if (!hasPermissions()) {
            ActivityCompat.requestPermissions(
                (this.context as Activity?)!!,
                permissions!!, permissionAll
            )
        }
    }

    fun hasPermissions(): Boolean {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M && context != null && permissions != null) {
            for (permission in permissions) {
                if (ActivityCompat.checkSelfPermission(
                        context,
                        permission
                    ) != PackageManager.PERMISSION_GRANTED
                ) {
                    return false
                }
            }
        }

        return true
    }
}
