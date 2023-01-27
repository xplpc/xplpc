package com.xplpc.runner.app

import android.os.Bundle
import androidx.appcompat.app.AppCompatActivity
import com.xplpc.client.LocalClient
import com.xplpc.client.RemoteClient
import com.xplpc.message.Param
import com.xplpc.message.Request
import com.xplpc.runner.R
import com.xplpc.runner.databinding.ActivityMainBinding
import com.xplpc.runner.extension.hideKeyboard
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch

class MainActivity : AppCompatActivity(), CoroutineScope {
    override val coroutineContext = Dispatchers.Main

    private lateinit var binding: ActivityMainBinding

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
    }

    private fun onBtSubmitForBatteryLevelClick() {
        val request = Request(
            "platform.battery.level",
            Param("suffix", "%")
        )

        LocalClient.call<String>(request) { response ->
            launch {
                val text = getString(R.string.battery_level_result, response)
                binding.tvBatteryLevelMessage.text = text
            }
        }
    }

    private fun onBtSubmitForLoginClick() {
        val request = Request(
            "sample.login",
            Param("username", binding.edFormUsername.text.toString()),
            Param("password", binding.edFormPassword.text.toString()),
            Param("remember", binding.ckRemember.isChecked),
        )

        RemoteClient.call<String>(request) { response ->
            launch {
                binding.tvFormMessage.text = getString(R.string.login_result, response)
            }
        }
    }
}
