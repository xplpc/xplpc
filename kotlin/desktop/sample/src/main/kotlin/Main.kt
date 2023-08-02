import androidx.compose.desktop.ui.tooling.preview.Preview
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.material.Button
import androidx.compose.material.ButtonDefaults
import androidx.compose.material.Checkbox
import androidx.compose.material.Divider
import androidx.compose.material.Icon
import androidx.compose.material.MaterialTheme
import androidx.compose.material.Text
import androidx.compose.material.TextField
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Lock
import androidx.compose.material.icons.filled.Person
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberCoroutineScope
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.TextStyle
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.input.PasswordVisualTransformation
import androidx.compose.ui.text.input.TextFieldValue
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.compose.ui.window.Window
import androidx.compose.ui.window.application
import com.xplpc.client.Client
import com.xplpc.message.Param
import com.xplpc.message.Request
import com.xplpc.platform.PlatformInitializer
import custom.Mapping
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch

@Composable
@Preview
fun App() {
    val scope = rememberCoroutineScope()

    MaterialTheme {
        var batteryLevelResponse by remember { mutableStateOf("Press Button To Execute") }
        var loginResponse by remember { mutableStateOf("Press Button To Execute") }

        var usernameTextController by remember { mutableStateOf(TextFieldValue()) }
        var passwordTextController by remember { mutableStateOf(TextFieldValue()) }
        var rememberMe by remember { mutableStateOf(false) }

        Column(
            modifier = Modifier.padding(16.dp).fillMaxWidth(),
            horizontalAlignment = Alignment.CenterHorizontally
        ) {
            Text(
                text = "BATTERY LEVEL",
                style = TextStyle(
                    color = Color.Black,
                    fontWeight = FontWeight.Bold,
                    fontSize = 20.sp,
                ),
                textAlign = TextAlign.Center,
            )
            Spacer(modifier = Modifier.height(16.dp))
            Text(
                text = batteryLevelResponse,
                style = TextStyle(
                    color = Color.Black,
                    fontWeight = FontWeight.Normal,
                    fontSize = 12.sp,
                ),
                textAlign = TextAlign.Center,
            )
            Spacer(modifier = Modifier.height(16.dp))
            Button(
                onClick = {
                    batteryLevelResponse = "Loading..."

                    scope.launch(Dispatchers.IO) {
                        val request = Request("platform.battery.level", Param("suffix", "%"))

                        Client.call<String>(request) { response ->
                            batteryLevelResponse = response ?: "ERROR"
                        }
                    }
                },
                colors = ButtonDefaults.textButtonColors(contentColor = Color.White, backgroundColor = Color.Blue)
            ) {
                Text(text = "SUBMIT")
            }
            Spacer(modifier = Modifier.height(16.dp))
            Divider(color = Color.Black.copy(alpha = 0.12f))
            Spacer(modifier = Modifier.height(16.dp))
            Text(
                text = "LOGIN",
                style = TextStyle(
                    color = Color.Black,
                    fontWeight = FontWeight.Bold,
                    fontSize = 20.sp,
                ),
                textAlign = TextAlign.Center,
            )
            Spacer(modifier = Modifier.height(16.dp))
            TextField(
                value = usernameTextController,
                onValueChange = { usernameTextController = it },
                label = { Text("Your username") },
                leadingIcon = { Icon(Icons.Filled.Person, contentDescription = null) },
            )
            Spacer(modifier = Modifier.height(16.dp))
            TextField(
                value = passwordTextController,
                onValueChange = { passwordTextController = it },
                label = { Text("Your password") },
                leadingIcon = { Icon(Icons.Filled.Lock, contentDescription = null) },
                visualTransformation = PasswordVisualTransformation(),
            )
            Spacer(modifier = Modifier.height(16.dp))
            Row(
                verticalAlignment = Alignment.CenterVertically,
            ) {
                Checkbox(
                    checked = rememberMe,
                    onCheckedChange = { rememberMe = it },
                )
                Text(
                    text = "Remember",
                    style = TextStyle(
                        color = Color.Black,
                        fontSize = 12.sp,
                    ),
                )
            }
            Spacer(modifier = Modifier.height(16.dp))
            Text(
                text = loginResponse,
                style = TextStyle(
                    color = Color.Black,
                    fontWeight = FontWeight.Normal,
                    fontSize = 12.sp,
                ),
                textAlign = TextAlign.Center,
            )
            Spacer(modifier = Modifier.height(16.dp))
            Button(
                onClick = {
                    loginResponse = "Loading..."

                    scope.launch(Dispatchers.IO) {
                        val request = Request(
                            "sample.login",
                            Param("username", usernameTextController.text),
                            Param("password", passwordTextController.text),
                            Param("remember", rememberMe)
                        )

                        Client.call<String>(request) { response ->
                            loginResponse = response ?: "ERROR"
                        }
                    }
                },
                colors = ButtonDefaults.textButtonColors(contentColor = Color.White, backgroundColor = Color.Blue)
            ) {
                Text(text = "SUBMIT")
            }
            Spacer(modifier = Modifier.height(16.dp))
        }
    }
}

fun main() = application {
    Window(
        onCloseRequest = ::exitApplication,
        title = "Runner",
    ) {
        PlatformInitializer.initialize()
        Mapping.initialize()
        App()
    }
}
