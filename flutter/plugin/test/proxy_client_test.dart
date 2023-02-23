import 'package:test/test.dart';
import 'package:xplpc/client/proxy_client.dart';
import 'package:xplpc/core/config.dart';
import 'package:xplpc/core/xplpc.dart';
import 'package:xplpc/data/mapping_list.dart';
import 'package:xplpc/map/mapping_item.dart';
import 'package:xplpc/message/message.dart';
import 'package:xplpc/message/param.dart';
import 'package:xplpc/message/request.dart';
import 'package:xplpc/message/response.dart';
import 'package:xplpc/serializer/json_serializer.dart';

void batteryLevel(Message m, Response r) {
  var suffix = m.get("suffix");
  r("100$suffix");
}

void main() {
  group('Testing Proxy Client', () {
    setUp(() {
      // initialize xplpc library
      XPLPC.instance.initialize(
        Config(
          JsonSerializer(),
        ),
      );
    });

    test('Battery Level', () {
      MappingList.instance.add(
        "platform.battery.level",
        MappingItem(batteryLevel),
      );

      var request = Request(
        "platform.battery.level",
        [
          Param("suffix", "%"),
        ],
      );

      ProxyClient.call(request.data(), (response) {
        expect("{\"r\":\"100%\"}", response);
      });
    });

    test('Battery Level Async', () async {
      MappingList.instance.add(
        "platform.battery.level",
        MappingItem(batteryLevel),
      );

      var request = Request(
        "platform.battery.level",
        [
          Param("suffix", "%"),
        ],
      );

      ProxyClient.call(request.data(), (response) {
        expect("{\"r\":\"100%\"}", response);
      });
    });
  });
}
