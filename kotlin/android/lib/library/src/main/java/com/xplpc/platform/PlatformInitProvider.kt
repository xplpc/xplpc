package com.xplpc.platform

import android.content.ContentProvider
import android.content.ContentValues
import android.content.pm.ApplicationInfo
import android.database.Cursor
import android.net.Uri
import com.xplpc.core.Config
import com.xplpc.core.XPLPC
import com.xplpc.serializer.JsonSerializer

class PlatformInitProvider : ContentProvider() {
    override fun onCreate(): Boolean {
        var debug = false

        context?.let {
            debug = it.applicationInfo.flags and ApplicationInfo.FLAG_DEBUGGABLE != 0
        }

        XPLPC.initialize(
            Config(debug, JsonSerializer()),
        )

        return true
    }

    override fun query(
        uri: Uri,
        projection: Array<String>?,
        selection: String?,
        selectionArgs: Array<String>?,
        sortOrder: String?
    ): Cursor? {
        return null
    }

    override fun getType(uri: Uri): String? {
        return null
    }

    override fun insert(uri: Uri, values: ContentValues?): Uri? {
        return null
    }

    override fun delete(uri: Uri, selection: String?, selectionArgs: Array<out String>?): Int {
        return 0
    }

    override fun update(
        uri: Uri,
        values: ContentValues?,
        selection: String?,
        selectionArgs: Array<out String>?
    ): Int {
        return 0
    }
}
