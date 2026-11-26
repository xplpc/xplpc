package com.xplpc.runner.activity

import android.content.Intent
import android.content.pm.PackageManager
import android.os.Bundle
import androidx.appcompat.app.AppCompatActivity
import androidx.lifecycle.lifecycleScope
import com.xplpc.client.Client
import com.xplpc.message.Param
import com.xplpc.message.Request
import com.xplpc.runner.R
import com.xplpc.runner.databinding.ActivityMainBinding
import com.xplpc.runner.extension.hideKeyboard
import com.xplpc.runner.permission.PermissionHandler
import kotlinx.coroutines.launch

// An answer can arrive after the activity is gone, so it reaches the views through the lifecycle scope, which is cancelled with the activity.
class MainActivity : AppCompatActivity() {
    private lateinit var binding: ActivityMainBinding

    private val permissions = arrayOf(android.Manifest.permission.CAMERA)

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
            lifecycleScope.launch {
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
            lifecycleScope.launch {
                binding.tvFormMessage.text = getString(R.string.login_result, response)
            }
        }
    }

    // The handler asks for whatever is missing, and the camera then opens from the answer to that request.
    private fun onBtSubmitForCameraClick() {
        if (PermissionHandler(this, permissions).hasPermissions()) {
            openCamera()
        }
    }

    // A denied permission is left denied, since asking again from here would ask again forever once the system stops showing the dialog.
    override fun onRequestPermissionsResult(
        requestCode: Int,
        permissions: Array<String>,
        grantResults: IntArray
    ) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)

        if (grantResults.isNotEmpty() && grantResults.all { it == PackageManager.PERMISSION_GRANTED }) {
            openCamera()
        }
    }

    private fun openCamera() {
        startActivity(Intent(this, CameraActivity::class.java))
    }
}
