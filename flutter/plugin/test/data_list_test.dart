import 'dart:ffi' as ffi;
import 'dart:typed_data';

import 'package:ffi/ffi.dart';
import 'package:logging/logging.dart';
import 'package:test/test.dart';
import 'package:xplpc/client/client.dart';
import 'package:xplpc/core/config.dart';
import 'package:xplpc/core/xplpc.dart';
import 'package:xplpc/data/mapping_list.dart';
import 'package:xplpc/helper/byte_array_helper.dart';
import 'package:xplpc/layer/layer.dart';
import 'package:xplpc/map/mapping_item.dart';
import 'package:xplpc/message/message.dart';
import 'package:xplpc/serializer/json_serializer.dart';
import 'package:xplpc/type/dataview.dart';

void main() {
  group('Testing Data Lists', () {
    setUpAll(() {
      XPLPC.instance.initialize(Config(JsonSerializer()));
    });

    tearDown(() {
      MappingList.instance.clear();

      expect(Layer.pending, 0);
      expect(Layer.buffers, 0);
    });

    test('Mapping List Clears', () {
      // Clearing retracts every name, so the router stops handing them over as well as the list forgetting them.

      const request = '{"f":"sample.temporary","p":[]}';

      MappingList.instance.add(
        "sample.temporary",
        MappingItem((m, r) => r("temporary")),
      );

      expect(MappingList.instance.has("sample.temporary"), true);
      expect(Client.callSyncFromString(request), '{"r":"temporary"}');

      MappingList.instance.clear();

      // A name still routed here would be answered empty too, so what tells the two apart is that this isolate is never asked.
      final failures = <LogRecord>[];
      final subscription = Logger('XPLPC').onRecord
          .where((r) => r.level >= Level.SEVERE)
          .listen(failures.add);

      try {
        expect(MappingList.instance.has("sample.temporary"), false);
        expect(Client.callSyncFromString(request), "");
      } finally {
        subscription.cancel();
      }

      expect(failures, isEmpty);
    });

    test('Mapping List Counts Entries', () {
      MappingList.instance.add("a", MappingItem((m, r) => r("")));
      MappingList.instance.add("b", MappingItem((m, r) => r("")));

      expect(MappingList.instance.count(), 2);
    });

    test('Message Returns Null For Another Type', () {
      final message = Message();
      message.set("number", 10);

      expect(message.get<int>("number"), 10);
      expect(message.get<String>("number"), null);
      expect(message.get<int>("missing"), null);
    });

    test('Message Reads A Number Only As A Type That Holds It Exactly', () {
      // Json carries one kind of number, so an integer is a double too when the double holds it exactly, and nothing else crosses between kinds.

      final message = Message();
      message.set("integer", 42);
      message.set("wide", 9007199254740993);
      message.set("fraction", 42.5);
      message.set("flag", true);
      message.set("text", "42");

      expect(message.get<double>("integer"), 42.0);
      expect(message.get<num>("integer"), 42);
      expect(message.get<double>("wide"), null);
      expect(message.get<int>("wide"), 9007199254740993);
      expect(message.get<int>("fraction"), null);
      expect(message.get<double>("fraction"), 42.5);
      expect(message.get<int>("flag"), null);
      expect(message.get<bool>("integer"), null);
      expect(message.get<int>("text"), null);
    });

    test('A Data View Naming Nothing Reads As Nothing', () {
      // A handle of zero names no buffer, and one the table never handed out is refused rather than read.

      expect(ByteArrayHelper.createFromDataView(DataView(0)), isEmpty);
      expect(ByteArrayHelper.createFromDataView(DataView(128)), isEmpty);
    });

    test('A Data View Naming Nothing Has Nothing To Release', () {
      // A view naming nothing is what a failed registration answers, so disposing it is not a second failure.

      expect(DataView(0).dispose(), false);
      expect(DataView(128).dispose(), false);
    });

    test('A Data View Reads Its Bytes Until It Is Disposed', () {
      final dataView = DataView.allocateFromByteArray(
        Uint8List.fromList([1, 2, 3]),
      );

      expect(Layer.buffers, 1);
      expect(ByteArrayHelper.createFromDataView(dataView), [1, 2, 3]);

      expect(dataView.dispose(), true);

      expect(Layer.buffers, 0);
      expect(ByteArrayHelper.createFromDataView(dataView), isEmpty);
    });

    test(
      'A Received Data View Cannot Dispose A Buffer This Side Registered',
      () {
        // A view that comes back through the wire names the same handle, and only the view that registered it can let it go.

        final dataView = DataView.allocateFromByteArray(
          Uint8List.fromList([1]),
        );
        final received = DataView.fromJson(dataView.toJson());

        expect(received.dispose(), false);
        expect(ByteArrayHelper.createFromDataView(dataView), [1]);
        expect(dataView.dispose(), true);
      },
    );

    test('A Data View Is Disposed Once', () {
      // Only the side that registered a buffer releases it, and a second release finds nothing registered.

      final dataView = DataView.allocateFromByteArray(Uint8List.fromList([1]));

      expect(dataView.dispose(), true);
      expect(dataView.dispose(), false);
    });

    test('A Borrowed Data View Leaves The Memory To Its Owner', () {
      // Disposing a borrowed view only releases the registration, so the owner's bytes are still there to read afterwards.

      final memory = calloc<ffi.Uint8>(2);

      try {
        memory.asTypedList(2).setAll(0, [7, 9]);

        final dataView = DataView.createFromByteBuffer(memory, 2);

        expect(ByteArrayHelper.createFromDataView(dataView), [7, 9]);
        expect(dataView.dispose(), true);
        expect(memory.asTypedList(2), [7, 9]);
      } finally {
        calloc.free(memory);
      }
    });

    test('A Data View Read Is A Copy', () {
      // The owner may release the bytes right after a read, so what a reader holds cannot alias them.

      final dataView = DataView.allocateFromByteArray(Uint8List.fromList([5]));

      try {
        final bytes = ByteArrayHelper.createFromDataView(dataView);
        bytes[0] = 6;

        expect(ByteArrayHelper.createFromDataView(dataView), [5]);
      } finally {
        dataView.dispose();
      }
    });

    test('A Data View Travels As A Handle', () {
      final dataView = DataView.allocateFromByteArray(Uint8List.fromList([1]));

      try {
        expect(dataView.toJson(), {'buffer': dataView.buffer});
        expect(DataView.fromJson(dataView.toJson()).buffer, dataView.buffer);
      } finally {
        dataView.dispose();
      }
    });

    test('A Value That Is Not A Data View Names Nothing', () {
      // The wire carries whatever the other side wrote, so a decoder answers rather than raising, and an address is not a handle.

      final values = <Object?>[
        <String, dynamic>{'buffer': true},
        <String, dynamic>{'buffer': -1},
        <String, dynamic>{'buffer': 1.5},
        <String, dynamic>{'buffer': '1'},
        <String, dynamic>{'ptr': 128, 'size': 4},
        <String, dynamic>{},
        null,
        42,
      ];

      for (final value in values) {
        final records = <LogRecord>[];
        final subscription = Logger('XPLPC').onRecord
            .where((r) => r.level >= Level.SEVERE)
            .listen(records.add);

        final view = DataView.fromJson(value);

        subscription.cancel();

        expect(view.buffer, 0, reason: '$value');
        expect(records.length, 1, reason: '$value');
        expect(records.single.message, contains('is not a data view'));
      }
    });
  });
}
