package com.xplpc.runner.activity

import android.content.Intent
import android.content.pm.PackageManager
import android.os.Bundle
import androidx.appcompat.app.AppCompatActivity
import androidx.core.app.ActivityCompat
import com.xplpc.client.Client
import com.xplpc.message.Param
import com.xplpc.message.Request
import com.xplpc.runner.R
import com.xplpc.runner.databinding.ActivityMainBinding
import com.xplpc.runner.extension.hideKeyboard
import com.xplpc.runner.permission.PermissionHandler
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch

class MainActivity : AppCompatActivity(), CoroutineScope {
    override val coroutineContext = Dispatchers.Main
    private lateinit var binding: ActivityMainBinding

    private val permissions = arrayOf(android.Manifest.permission.CAMERA)
    private val permissionCode = 100
    private var intentForPermission: Intent? = null

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)

        binding.container.setOnClickListener {
            it.hideKeyboard()
        }

        binding.btSubmitForBatteryLevel.setOnClickListener {
            onBtSubmitForBatteryLevelClick()
        }

        binding.btSubmitForLogin.setOnClickListener {
            onBtSubmitForLoginClick()
        }

        binding.btSubmitForCamera.setOnClickListener {
            onBtSubmitForCameraClick()
        }
    }

    private fun onBtSubmitForBatteryLevelClick() {
        val request =
            Request(
                "platform.battery.level",
                Param("suffix", "%")
            )

        Client.call<String>(request) { response ->
            launch {
                val text = getString(R.string.battery_level_result, response)
                binding.tvBatteryLevelMessage.text = text
            }
        }
    }

    private fun onBtSubmitForLoginClick() {
        val request =
            Request(
                "sample.login",
                Param("username", binding.edFormUsername.text.toString()),
                Param("password", binding.edFormPassword.text.toString()),
                Param("remember", binding.ckRemember.isChecked),
            )

        Client.call<String>(request) { response ->
            launch {
                binding.tvFormMessage.text = getString(R.string.login_result, response)
            }
        }
    }

    private fun onBtSubmitForCameraClick() {
        intentForPermission =
            Intent(this, CameraActivity::class.java).apply {
                // you can add values(if any) to pass to the next class or avoid using `.apply`
            }

        checkPermissions(permissions)
    }

    private fun checkPermissions(permissions: Array<String>) {
        val runtimePermissions = PermissionHandler(this, permissions)
        val gotPermissions = runtimePermissions.hasPermissions()

        if (gotPermissions) {
            intentForPermission?.let {
                startActivity(it)
            }
        } else {
            ActivityCompat.requestPermissions(this, permissions, permissionCode)
        }
    }

    override fun onRequestPermissionsResult(
        requestCode: Int,
        permissions: Array<String>,
        grantResults: IntArray
    ) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)

        for (i in permissions.indices) {
            if (grantResults[i] != PackageManager.PERMISSION_GRANTED) {
                checkPermissions(permissions)
            }
        }

        if ((permissions.isNotEmpty()) && (!grantResults.contains(PackageManager.PERMISSION_DENIED))) {
            intentForPermission?.let {
                startActivity(it)
            }
        }
    }
}
