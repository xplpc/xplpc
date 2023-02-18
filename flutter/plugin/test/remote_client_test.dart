import 'package:test/test.dart';
import 'package:xplpc/client/remote_client.dart';
import 'package:xplpc/core/config.dart';
import 'package:xplpc/core/xplpc.dart';
import 'package:xplpc/data/mapping_list.dart';
import 'package:xplpc/map/mapping_item.dart';
import 'package:xplpc/message/message.dart';
import 'package:xplpc/message/param.dart';
import 'package:xplpc/message/request.dart';
import 'package:xplpc/message/response.dart';
import 'package:xplpc/serializer/json_serializer.dart';

void reverse(Message m, Response r) {
  r("ok");
}

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

    test('Login Async', () async {
      var request = Request("sample.login", [
        Param("username", "paulo"),
        Param("password", "123456"),
        Param("remember", true),
      ]);

      RemoteClient.call<String>(request, (response) {
        expect("LOGGED-WITH-REMEMBER", response);
      });
    });

    test('Login Invalid Cast', () {
      var request = Request("sample.login", [
        Param("username", "paulo"),
        Param("password", "123456"),
        Param("remember", true),
      ]);

      RemoteClient.call<bool>(request, (response) {
        expect(null, response);
      });
    });

    test('Reverse', () {
      MappingList.instance.add(
        "platform.reverse.response",
        MappingItem(reverse),
      );

      var request = Request("sample.reverse");

      RemoteClient.call<String>(request, (response) {
        expect("response-is-ok", response);
      });
    });

    test('Reverse Async', () async {
      MappingList.instance.add(
        "platform.reverse.response",
        MappingItem(reverse),
      );

      var request = Request("sample.reverse");

      RemoteClient.call<String>(request, (response) {
        expect("response-is-ok", response);
      });
    });
  });
}
