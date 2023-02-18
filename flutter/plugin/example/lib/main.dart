import 'package:flutter/material.dart';
import 'package:xplpc/client/local_client.dart';
import 'package:xplpc/client/remote_client.dart';
import 'package:xplpc/core/config.dart';
import 'package:xplpc/core/xplpc.dart';
import 'package:xplpc/message/param.dart';
import 'package:xplpc/message/request.dart';
import 'package:xplpc/serializer/json_serializer.dart';
import 'package:xplpc_example/custom/mapping.dart';

void main() {
  // load library
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
  @override
  void initState() {
    super.initState();
  }

  void onBatteryLevelSubmitButtonClick() {
    var request = Request(
      "platform.battery.level",
      [
        Param("suffix", "%"),
      ],
    );

    LocalClient.call<String>(request, (response) {
      print(response);
    });
  }

  void onLoginSubmitButtonClick() {
    var request = Request("sample.login", [
      Param("username", "paulo"),
      Param("password", "123456"),
      Param("remember", true),
    ]);

    RemoteClient.call<String>(request, (response) {
      print(response);
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
            padding: const EdgeInsets.all(10),
            child: Column(
              children: [
                TextButton(
                  style: ButtonStyle(
                    foregroundColor:
                        MaterialStateProperty.all<Color>(Colors.blue),
                  ),
                  onPressed: () {
                    //onBatteryLevelSubmitButtonClick();
                    onLoginSubmitButtonClick();
                  },
                  child: const Text('SUBMIT'),
                ),
              ],
            ),
          ),
        ),
      ),
    );
  }
}
