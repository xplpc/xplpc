import 'package:logging/logging.dart';
import 'package:test/test.dart';
import 'package:xplpc/core/config.dart';
import 'package:xplpc/core/xplpc.dart';
import 'package:xplpc/data/callback_list.dart';
import 'package:xplpc/helper/byte_array_helper.dart';
import 'package:xplpc/data/mapping_list.dart';
import 'package:xplpc/map/mapping_item.dart';
import 'package:xplpc/message/message.dart';
import 'package:xplpc/type/dataview.dart';
import 'package:xplpc/serializer/json_serializer.dart';
import 'package:xplpc/util/unique_id.dart';

void main() {
  group('Testing Data Lists', () {
    setUpAll(() {
      XPLPC.instance.initialize(Config(JsonSerializer()));
    });

    tearDown(() {
      CallbackList.instance.clear();
      MappingList.instance.clear();
    });

    test('Callback List Executes Only Once', () {
      var calls = 0;

      CallbackList.instance.add("callback-once", (_) => calls += 1);

      CallbackList.instance.execute("callback-once", "");
      CallbackList.instance.execute("callback-once", "");

      expect(calls, 1);
    });

    test('Callback List Registers Synchronously', () {
      CallbackList.instance.add("callback-sync", (_) {});

      expect(CallbackList.instance.count(), 1);
    });

    test('Callback List Removes Pending Entry', () {
      var calls = 0;

      CallbackList.instance.add("callback-removed", (_) => calls += 1);
      CallbackList.instance.remove("callback-removed");
      CallbackList.instance.execute("callback-removed", "");

      expect(calls, 0);
      expect(CallbackList.instance.count(), 0);
    });

    test('Callback List Ignores Unknown Key', () {
      CallbackList.instance.execute("callback-unknown", "");

      expect(CallbackList.instance.count(), 0);
    });

    test('Mapping List Clears', () {
      MappingList.instance.add(
        "sample.temporary",
        MappingItem((m, r) => r("")),
      );

      expect(MappingList.instance.has("sample.temporary"), true);

      MappingList.instance.clear();

      expect(MappingList.instance.has("sample.temporary"), false);
    });

    test('Unique ID Generates Distinct Keys', () {
      final keys = List.generate(1000, (_) => UniqueID.generate());

      expect(keys.toSet().length, 1000);
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

    test('Empty Data View Reads As Nothing', () {
      // A view that carries no address describes nothing to read, and reading it must not take the isolate down.

      expect(ByteArrayHelper.createFromDataView(DataView(0, 16)).length, 0);
      expect(ByteArrayHelper.createFromDataView(DataView(128, 0)).length, 0);
      expect(ByteArrayHelper.createFromDataView(DataView(128, -1)).length, 0);
    });
    test('A Value That Is Not A Data View Is Answered With The Empty One', () {
      // The wire carries whatever the other side wrote, so a decoder answers rather than raising.

      final records = <String>[];
      final subscription = Logger(
        'XPLPC',
      ).onRecord.listen((record) => records.add(record.message));

      final view = DataView.fromJson(<String, dynamic>{'ptr': true, 'size': 1});

      subscription.cancel();

      expect(view.ptr, 0);
      expect(view.size, 0);
      expect(records.any((m) => m.contains('is not a data view')), true);
    });
  });
}
