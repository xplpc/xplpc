import 'package:flutter/material.dart';
import 'package:xplpc/client/client.dart';
import 'package:xplpc/core/config.dart';
import 'package:xplpc/core/xplpc.dart';
import 'package:xplpc/message/param.dart';
import 'package:xplpc/message/request.dart';
import 'package:xplpc/serializer/json_serializer.dart';
import 'package:xplpc_example/custom/mapping.dart';

import 'main.reflectable.dart';

void main() {
  // initialize reflectable
  initializeReflectable();

  // initialize xplpc library
  XPLPC.instance.initialize(
    Config(
      JsonSerializer(),
    ),
  );

  // mapping
  Mapping.instance.initialize();

  // run application
  runApp(const MyApp());
}

class MyApp extends StatefulWidget {
  const MyApp({super.key});

  @override
  State<MyApp> createState() => _MyAppState();
}

class _MyAppState extends State<MyApp> {
  String batteryLevelResponse = "Press Button To Execute";
  String loginResponse = "Press Button To Execute";

  TextEditingController usernameTextController = TextEditingController();
  TextEditingController passwordTextController = TextEditingController();
  bool rememberMe = false;

  @override
  void initState() {
    super.initState();
  }

  void onBatteryLevelSubmitButtonClick() {
    final request = Request(
      "platform.battery.level",
      [
        Param("suffix", "%"),
      ],
    );

    Client.call<String>(request, (response) {
      setState(() {
        batteryLevelResponse = "Response: $response";
      });
    });
  }

  void onLoginSubmitButtonClick() {
    final request = Request("sample.login", [
      Param("username", usernameTextController.text),
      Param("password", passwordTextController.text),
      Param("remember", rememberMe),
    ]);

    Client.call<String>(request, (response) {
      setState(() {
        loginResponse = "Response: $response";
      });
    });
  }

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      home: Scaffold(
        appBar: AppBar(
          title: const Text('XPLPC'),
        ),
        body: SingleChildScrollView(
          child: Container(
            width: double.infinity,
            padding: const EdgeInsets.all(16),
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.center,
              mainAxisAlignment: MainAxisAlignment.center,
              children: [
                const Text(
                  "BATTERY LEVEL",
                  textAlign: TextAlign.center,
                  style: TextStyle(
                    color: Colors.black,
                    fontWeight: FontWeight.bold,
                    fontSize: 20,
                  ),
                ),
                const SizedBox(height: 16),
                Text(
                  batteryLevelResponse,
                  textAlign: TextAlign.center,
                  style: const TextStyle(
                    color: Colors.black54,
                    fontWeight: FontWeight.normal,
                    fontSize: 12,
                  ),
                ),
                const SizedBox(height: 16),
                TextButton(
                  style: ButtonStyle(
                    foregroundColor: MaterialStateProperty.all<Color>(
                      Colors.blue,
                    ),
                  ),
                  onPressed: () {
                    onBatteryLevelSubmitButtonClick();
                  },
                  child: const Text('SUBMIT'),
                ),
                const SizedBox(height: 16),
                const Divider(color: Colors.black12),
                const SizedBox(height: 16),
                const Text(
                  "LOGIN",
                  textAlign: TextAlign.center,
                  style: TextStyle(
                    color: Colors.black,
                    fontWeight: FontWeight.bold,
                    fontSize: 20,
                  ),
                ),
                const SizedBox(height: 16),
                TextField(
                  controller: usernameTextController,
                  decoration: const InputDecoration(
                    border: UnderlineInputBorder(),
                    icon: Icon(Icons.person),
                    hintText: 'Your username',
                  ),
                  style: const TextStyle(
                    fontSize: 14,
                  ),
                ),
                const SizedBox(height: 16),
                TextField(
                  controller: passwordTextController,
                  obscureText: true,
                  obscuringCharacter: "*",
                  decoration: const InputDecoration(
                    border: UnderlineInputBorder(),
                    icon: Icon(Icons.key),
                    hintText: 'Your password',
                  ),
                  style: const TextStyle(
                    fontSize: 14,
                  ),
                ),
                const SizedBox(height: 16),
                CheckboxListTile(
                  value: rememberMe,
                  title: const Text(
                    "Remember",
                    style: TextStyle(
                      fontSize: 12,
                      color: Colors.black87,
                    ),
                  ),
                  controlAffinity: ListTileControlAffinity.leading,
                  onChanged: (bool? value) {
                    setState(() {
                      rememberMe = value ?? false;
                    });
                  },
                ),
                const SizedBox(height: 16),
                Text(
                  loginResponse,
                  textAlign: TextAlign.center,
                  style: const TextStyle(
                    color: Colors.black54,
                    fontWeight: FontWeight.normal,
                    fontSize: 12,
                  ),
                ),
                const SizedBox(height: 16),
                TextButton(
                  style: ButtonStyle(
                    foregroundColor: MaterialStateProperty.all<Color>(
                      Colors.blue,
                    ),
                  ),
                  onPressed: () {
                    onLoginSubmitButtonClick();
                  },
                  child: const Text('SUBMIT'),
                ),
                const SizedBox(height: 16),
              ],
            ),
          ),
        ),
      ),
    );
  }
}
