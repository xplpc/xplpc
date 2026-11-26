import 'dart:isolate';

import 'package:test/test.dart';
import 'package:xplpc/client/client.dart';
import 'package:xplpc/core/config.dart';
import 'package:xplpc/core/xplpc.dart';
import 'package:xplpc/data/mapping_list.dart';
import 'package:xplpc/layer/layer.dart';
import 'package:xplpc/map/mapping_item.dart';
import 'package:xplpc/serializer/json_serializer.dart';

void main() {
  group('Testing Isolate', () {
    test('A Layer Whose Isolate Exits Is Never Reached Again', () async {
      // The isolate leaves a call waiting on an answer that arrives a second later, and a name nobody else serves, and every callable it registered dies with it.

      await Isolate.run(() {
        XPLPC.instance.initialize(Config(JsonSerializer()));

        MappingList.instance.add(
          "isolate.exited",
          MappingItem((m, r) => r("from-exited")),
        );

        Client.callFromString('{"f":"sample.async","p":[]}', (_) {});
      });

      await Future<void>.delayed(const Duration(milliseconds: 1500));

      // This isolate attaches only now, so nothing it does could have taken the place of the layer that exited.
      XPLPC.instance.initialize(Config(JsonSerializer()));

      expect(Client.callSyncFromString('{"f":"isolate.exited","p":[]}'), "");

      expect(Layer.pending, 0);
    });
  });
}
