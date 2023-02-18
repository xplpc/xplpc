import 'package:test/test.dart';
import 'package:xplpc/client/remote_client.dart';
import 'package:xplpc/core/config.dart';
import 'package:xplpc/core/xplpc.dart';
import 'package:xplpc/message/param.dart';
import 'package:xplpc/message/request.dart';
import 'package:xplpc/serializer/json_serializer.dart';

void main() {
  group('Testing Remote Client', () {
    setUp(() {
      XPLPC.instance.initialize(
        Config(
          JsonSerializer(),
        ),
      );
    });

    test('Login', () {
      var request = Request("sample.login", [
        Param("username", "paulo"),
        Param("password", "123456"),
        Param("remember", true),
      ]);

      RemoteClient.call<String>(request, (response) {
        expect("LOGGED-WITH-REMEMBER", response);
      });
    });
  });
}
