import 'dart:async';
import 'dart:typed_data';

import 'package:logging/logging.dart';
import 'package:test/test.dart';
import 'package:xplpc/client/client.dart';
import 'package:xplpc/core/config.dart';
import 'package:xplpc/core/xplpc.dart';
import 'package:xplpc/data/callback_list.dart';
import 'package:xplpc/data/mapping_list.dart';
import 'package:xplpc/helper/byte_array_helper.dart';
import 'package:xplpc/map/mapping_item.dart';
import 'package:xplpc/message/message.dart';
import 'package:xplpc/message/param.dart';
import 'package:xplpc/message/request.dart';
import 'package:xplpc/message/response.dart';
import 'package:xplpc/proxy/platform_proxy.dart';
import 'package:xplpc/serializer/json_serializer.dart';
import 'package:xplpc/type/dataview.dart';

import 'client_test.reflectable.dart';

void batteryLevel(Message m, Response r) {
  final suffix = m.get<String>("suffix");
  r("100$suffix");
}

void deferredReverse(Message m, Response r) {
  Future.delayed(const Duration(milliseconds: 50), () => r("ok"));
}

void deferredAnswer(Message m, Response r) {
  Future.delayed(const Duration(milliseconds: 50), () => r("deferred"));
}

void mismatchedType(Message m, Response r) {
  final asString = m.get<String>("value");
  final asNumber = m.get<num>("value");

  r("${asString ?? "nil"}/${asNumber?.toInt() ?? -1}");
}

void reverse(Message m, Response r) {
  r("ok");
}

void verifyCall<T>(Request request, void Function(dynamic) block) {
  // An assertion that fails inside a callback cannot cross the native frame that invoked it, so it is captured and raised here.

  Object? failure;
  var answered = false;

  Client.call<T>(request, (response) {
    answered = true;

    try {
      block(response);
    } catch (e) {
      failure = e;
    }
  });

  expect(answered, isTrue, reason: "the callback was never invoked");

  if (failure != null) {
    throw failure!;
  }
}

void verifyCallFromString(String requestData, void Function(String) block) {
  Object? failure;
  var answered = false;

  Client.callFromString(requestData, (response) {
    answered = true;

    try {
      block(response);
    } catch (e) {
      failure = e;
    }
  });

  expect(answered, isTrue, reason: "the callback was never invoked");

  if (failure != null) {
    throw failure!;
  }
}

void main() {
  initializeReflectable();

  group('Testing Client', () {
    setUp(() {
      XPLPC.instance.initialize(Config(JsonSerializer()));
    });

    tearDown(() {
      // A leaked callback is the defect this project has found most often, so every test is held to leaving none.
      expect(CallbackList.instance.count(), 0);
    });

    test('Battery Level', () {
      MappingList.instance.add(
        "platform.battery.level",
        MappingItem(batteryLevel),
      );

      var request = Request("platform.battery.level", [Param("suffix", "%")]);

      verifyCall<String>(request, (response) {
        expect("100%", response);
      });
    });

    test('Battery Level Async', () async {
      MappingList.instance.add(
        "platform.battery.level",
        MappingItem(batteryLevel),
      );

      var request = Request("platform.battery.level", [Param("suffix", "%")]);

      verifyCall<String>(request, (response) {
        expect("100%", response);
      });
    });

    test('Battery Level Call Async', () async {
      MappingList.instance.add(
        "platform.battery.level",
        MappingItem(batteryLevel),
      );

      var request = Request("platform.battery.level", [Param("suffix", "%")]);

      String? response = await Client.callAsync<String>(request);
      expect("100%", response);
    });

    test('Login', () {
      var request = Request("sample.login", [
        Param("username", "paulo"),
        Param("password", "123456"),
        Param("remember", true),
      ]);

      verifyCall<String>(request, (response) {
        expect("LOGGED-WITH-REMEMBER", response);
      });
    });

    test('Login Async', () async {
      var request = Request("sample.login", [
        Param("username", "paulo"),
        Param("password", "123456"),
        Param("remember", true),
      ]);

      verifyCall<String>(request, (response) {
        expect("LOGGED-WITH-REMEMBER", response);
      });
    });

    test('Login Invalid Cast', () {
      var request = Request("sample.login", [
        Param("username", "paulo"),
        Param("password", "123456"),
        Param("remember", true),
      ]);

      verifyCall<bool>(request, (response) {
        expect(null, response);
      });
    });

    test('Reverse', () {
      MappingList.instance.add(
        "platform.reverse.response",
        MappingItem(reverse),
      );

      var request = Request("sample.reverse");

      verifyCall<String>(request, (response) {
        expect("response-is-ok", response);
      });
    });

    test('Reverse Async', () async {
      MappingList.instance.add(
        "platform.reverse.response",
        MappingItem(reverse),
      );

      var request = Request("sample.reverse");

      verifyCall<String>(request, (response) {
        expect("response-is-ok", response);
      });
    });

    test('Echoes Every Encoding Width', () async {
      // A string has to survive every width utf8 can encode it in, on the way out and on the way back.

      final samples = [
        "plain ascii",
        "caf\u00e9 na\u00efve",
        "\u4e2d\u6587\u30c6\u30b9\u30c8",
        "\u{1F600}\u{1F468}\u200d\u{1F469}",
        "mixed \u00e9 \u4e2d \u{1F600} end",
      ];

      for (final sample in samples) {
        final response = await Client.callAsync<String>(
          Request("sample.echo", [Param("value", sample)]),
        );

        expect(response, sample);
      }
    });

    test('Large Data View Crosses Without Truncating', () async {
      // A buffer far past what a small integer holds has to cross without being truncated anywhere on the way.

      const size = 4 * 1024 * 1024;
      final dataView = DataView.allocateFromByteArray(
        Uint8List.fromList(List.filled(size, 100)),
      );

      try {
        final response = await Client.callAsync<String>(
          Request("sample.image.grayscale.dataview", [
            Param("dataView", dataView),
          ]),
        );

        expect(response, "OK");
        expect(dataView.size, size);

        final bytes = ByteArrayHelper.createFromDataView(dataView);

        expect(bytes.length, size);
        expect(bytes[0], 100);
        expect(bytes[size - 1], 100);
      } finally {
        dataView.dispose();
      }
    });

    test('Todo Round Trip', () async {
      // An object crossing the bridge exercises the nested shapes the primitives never reach.

      final response = await Client.callAsync<Map<String, dynamic>>(
        Request("sample.todo.single", [
          Param("item", {
            "id": 1,
            "title": "Title 1",
            "body": "Body 1",
            "data": {"data1": "value1"},
            "done": true,
          }),
        ]),
      );

      expect(response?["id"], 1);
      expect(response?["title"], "Title 1");
      expect(response?["data"]["data1"], "value1");
      expect(response?["done"], true);
    });

    test('Core Mapping Answers From Its Own Thread', () async {
      // The mapping answers from a thread it created, which can only reach the isolate through its event loop.

      final response = await Client.callAsync<String>(Request("sample.async"));
      expect(response, null);
    });

    test('Async Calls Overlap', () async {
      // A mapping that answers later never holds the bridge, so calls to it overlap instead of running one after another.

      final start = DateTime.now();

      final responses = await Future.wait(
        List.generate(
          4,
          (_) => Client.callAsync<String>(Request("sample.async")),
        ),
      );

      final elapsed = DateTime.now().difference(start).inMilliseconds;

      expect(responses.length, 4);
      expect(elapsed, lessThan(3000));
    });

    test('Grayscale Image With DataView', () {
      Uint8List data = Uint8List.fromList([
        255,
        0,
        0,
        255,
        0,
        255,
        0,
        255,
        0,
        0,
        255,
        255,
        0,
        0,
        0,
        0,
      ]);

      var dataView = DataView.allocateFromByteArray(data);

      var request = Request("sample.image.grayscale.dataview", [
        Param("dataView", dataView),
      ]);

      verifyCall<String>(request, (response) {
        expect("OK", response);

        var data = ByteArrayHelper.createFromDataView(dataView);

        expect(16, dataView.size);
        expect(data[0].toInt(), 85);
        expect(data[4].toInt(), 85);
        expect(data[8].toInt(), 85);
        expect(data[12].toInt(), 0);
      });

      dataView.dispose();
    });

    test('Grayscale Image With DataView Async', () async {
      Uint8List data = Uint8List.fromList([
        255,
        0,
        0,
        255,
        0,
        255,
        0,
        255,
        0,
        0,
        255,
        255,
        0,
        0,
        0,
        0,
      ]);

      var dataView = DataView.allocateFromByteArray(data);

      var request = Request("sample.image.grayscale.dataview", [
        Param("dataView", dataView),
      ]);

      verifyCall<String>(request, (response) {
        expect("OK", response);

        var data = ByteArrayHelper.createFromDataView(dataView);

        expect(16, dataView.size);
        expect(data[0].toInt(), 85);
        expect(data[4].toInt(), 85);
        expect(data[8].toInt(), 85);
        expect(data[12].toInt(), 0);
      });

      dataView.dispose();
    });

    test('DataView', () {
      var request = Request("sample.dataview");

      verifyCall<DataView>(request, (response) {
        expect(false, response == null);

        final dataView = response as DataView;
        final originalData = ByteArrayHelper.createFromDataView(dataView);

        expect(16, dataView.size);
        expect(originalData[0].toInt(), 255);
        expect(originalData[3].toInt(), 255);
        expect(originalData[7].toInt(), 255);
        expect(originalData[12].toInt(), 0);

        var dataView2 = DataView.allocateFromByteArray(originalData);
        var request2 = Request("sample.image.grayscale.dataview", [
          Param("dataView", dataView2),
        ]);

        verifyCall<String>(request2, (response2) {
          expect("OK", response2);

          var processedData = ByteArrayHelper.createFromDataView(dataView2);

          expect(16, dataView.size);
          expect(processedData[0].toInt(), 85);
          expect(processedData[4].toInt(), 85);
          expect(processedData[8].toInt(), 85);
          expect(processedData[12].toInt(), 0);

          expect(16, originalData.length);
          expect(originalData[0].toInt(), 255);
          expect(originalData[3].toInt(), 255);
          expect(originalData[7].toInt(), 255);
          expect(originalData[12].toInt(), 0);
        });

        dataView2.dispose();
      });
    });

    test('DataView Async', () async {
      var request = Request("sample.dataview");

      verifyCall<DataView>(request, (response) {
        expect(false, response == null);

        final dataView = response as DataView;
        final originalData = ByteArrayHelper.createFromDataView(dataView);

        expect(16, dataView.size);
        expect(originalData[0].toInt(), 255);
        expect(originalData[3].toInt(), 255);
        expect(originalData[7].toInt(), 255);
        expect(originalData[12].toInt(), 0);

        var dataView2 = DataView.allocateFromByteArray(originalData);
        var request2 = Request("sample.image.grayscale.dataview", [
          Param("dataView", dataView2),
        ]);

        verifyCall<String>(request2, (response2) {
          expect("OK", response2);

          var processedData = ByteArrayHelper.createFromDataView(dataView2);

          expect(16, dataView.size);
          expect(processedData[0].toInt(), 85);
          expect(processedData[4].toInt(), 85);
          expect(processedData[8].toInt(), 85);
          expect(processedData[12].toInt(), 0);

          expect(16, originalData.length);
          expect(originalData[0].toInt(), 255);
          expect(originalData[3].toInt(), 255);
          expect(originalData[7].toInt(), 255);
          expect(originalData[12].toInt(), 0);
        });

        dataView2.dispose();
      });
    });

    test('Battery Level From String', () {
      MappingList.instance.add(
        "platform.battery.level",
        MappingItem(batteryLevel),
      );

      var request = Request("platform.battery.level", [Param("suffix", "%")]);

      verifyCallFromString(request.data(), (response) {
        expect("{\"r\":\"100%\"}", response);
      });
    });

    test('Battery Level Async From String', () async {
      MappingList.instance.add(
        "platform.battery.level",
        MappingItem(batteryLevel),
      );

      var request = Request("platform.battery.level", [Param("suffix", "%")]);

      verifyCallFromString(request.data(), (response) {
        expect("{\"r\":\"100%\"}", response);
      });
    });

    test('Battery Level Call Async From String', () async {
      MappingList.instance.add(
        "platform.battery.level",
        MappingItem(batteryLevel),
      );

      var request = Request("platform.battery.level", [Param("suffix", "%")]);

      String response = await Client.callAsyncFromString(request.data());
      expect("{\"r\":\"100%\"}", response);
    });
    Map<String, dynamic> allTypesItem() => {
      // The edges are what a serializer loses, since a small value round trips even when the format cannot carry the type.
      "typeInt8": -128,
      "typeInt16": -32768,
      "typeInt32": -2147483648,
      "typeInt64": 9007199254740993,
      "typeFloat32": 0.1,
      "typeFloat64": 0.1,
      "typeBool": true,
      "typeOptional": null,
      "typeList": <dynamic>[],
      "typeMap": {"item1": "ok"},
      "typeDateTime": 494938800,
      "typeChar": 122,
      "typeChar16": 174,
      "typeChar32": 174,
      "typeWchar": 174,
      "typeString": "ok",
    };

    test('All Types Single Item Keeps Every Edge', () async {
      final item = allTypesItem();
      item["typeList"] = [allTypesItem()];

      final response = await Client.callAsync<Map<String, dynamic>>(
        Request("sample.alltypes.single", [Param("item", item)]),
      );

      expect(response?["typeInt8"], -128);
      expect(response?["typeInt16"], -32768);
      expect(response?["typeInt32"], -2147483648);
      expect(response?["typeInt64"], 9007199254740993);
      expect((response?["typeFloat32"] as double) - 0.1 < 1e-7, true);
      expect(response?["typeFloat64"], 0.1);
      expect(response?["typeBool"], true);
      expect(response?["typeOptional"], null);
      expect((response?["typeList"] as List).length, 1);
      expect(response?["typeMap"]["item1"], "ok");
      expect(response?["typeDateTime"], 494938800);
      expect(response?["typeChar"], 122);
      expect(response?["typeChar16"], 174);
      expect(response?["typeChar32"], 174);
      expect(response?["typeWchar"], 174);
      expect(response?["typeString"], "ok");
    });

    test('All Types Multiple Items', () async {
      final response = await Client.callAsync<List<dynamic>>(
        Request("sample.alltypes.list", [
          Param("items", [allTypesItem(), allTypesItem()]),
        ]),
      );

      expect(response?.length, 2);
      expect(response?[0]["typeInt64"], 9007199254740993);
      expect(response?[1]["typeString"], "ok");
    });

    test('Todo Multiple Items', () async {
      final response = await Client.callAsync<List<dynamic>>(
        Request("sample.todo.list", [
          Param("items", [
            {
              "id": 1,
              "title": "Title 1",
              "body": "Body 1",
              "data": <String, dynamic>{},
              "done": true,
            },
            {
              "id": 2,
              "title": "Title 2",
              "body": "Body 2",
              "data": <String, dynamic>{},
              "done": true,
            },
          ]),
        ]),
      );

      expect(response?.length, 2);
      expect(response?[0]["title"], "Title 1");
      expect(response?[1]["title"], "Title 2");
    });
    test(
      'Nested Host Mapping Answering Later Reaches The Outer Caller',
      () async {
        MappingList.instance.add(
          "platform.reverse.response",
          MappingItem(deferredReverse),
        );

        final answered = Completer<String?>();

        Client.call<String>(Request("sample.reverse"), (response) {
          answered.complete(response);
        });

        expect(await answered.future, "response-is-ok");
      },
    );

    test('Host Mapping Answers After The Call Returned', () async {
      MappingList.instance.add(
        "platform.deferred.answer",
        MappingItem(deferredAnswer),
      );

      final answered = Completer<String?>();

      Client.call<String>(Request("platform.deferred.answer"), (response) {
        answered.complete(response);
      });

      expect(answered.isCompleted, false);
      expect(await answered.future, "deferred");
    });

    test('Mapping Reading A Mismatched Type Gets Nothing', () {
      // Reading a value as a type it does not hold answers nothing rather than a value read the wrong way.

      MappingList.instance.add(
        "platform.mismatched.type",
        MappingItem(mismatchedType),
      );

      var request = Request("platform.mismatched.type", [Param("value", 10)]);

      verifyCall<String>(request, (response) {
        expect("nil/10", response);
      });
    });
    test(
      'Unknown Function Answers Empty And Leaves Nothing Registered',
      () async {
        // Nothing owns this name, so the caller is answered with the empty value and the registration is not left behind.

        final before = CallbackList.instance.count();

        final response = await Client.callAsync<String>(Request("not.found"));

        expect(response, null);
        expect(CallbackList.instance.count(), before);
      },
    );
    test('Call Async Gives The Event Loop A Turn Before Answering', () async {
      // A call site has to behave the same whether the mapping is quick or slow, so the loop is reached even when the answer is already there.

      final order = <String>[];

      scheduleMicrotask(() => order.add("loop"));

      await Client.callAsync<String>(Request("sample.version"));
      order.add("after await");

      expect(order, ["loop", "after await"]);
    });

    test('Uninitialized Library Answers Empty', () async {
      // The library holds the callbacks from the moment it loads, so being reached before it is ready answers rather than hangs.

      XPLPC.instance.initialized = false;

      try {
        final response = await Client.callAsync<String>(
          Request("platform.battery.level", [Param("suffix", "%")]),
        );

        expect(response, null);

        expect(
          Client.callSync<String>(
            Request("platform.battery.level", [Param("suffix", "%")]),
          ),
          null,
        );
      } finally {
        XPLPC.instance.initialized = true;
      }
    });

    test('Call Sync Answers The Value From An Inline Mapping', () {
      // A mapping that answers before it returns has already resolved the key, so the value is there to be read.

      MappingList.instance.add(
        "platform.battery.level",
        MappingItem(batteryLevel),
      );

      var request = Request("platform.battery.level", [Param("suffix", "%")]);

      expect(Client.callSync<String>(request), "100%");
    });

    test('Call Sync Answers The Value From An Inline Native Mapping', () {
      var request = Request("sample.login", [
        Param("username", "paulo"),
        Param("password", "123456"),
        Param("remember", true),
      ]);

      expect(Client.callSync<String>(request), "LOGGED-WITH-REMEMBER");
    });

    test('Call Sync Answers Empty When The Mapping Defers', () async {
      // A mapping that answers later cannot be read synchronously, and the registration it would have resolved is dropped rather than left behind.

      MappingList.instance.add(
        "platform.deferred.answer",
        MappingItem(deferredAnswer),
      );

      final before = CallbackList.instance.count();
      final records = <LogRecord>[];

      hierarchicalLoggingEnabled = true;
      Logger('XPLPC').level = Level.ALL;

      final subscription = Logger('XPLPC').onRecord.listen(records.add);

      try {
        expect(
          Client.callSync<String>(Request("platform.deferred.answer")),
          null,
        );
        expect(
          records.any(
            (r) => r.message.contains("did not answer synchronously"),
          ),
          isTrue,
        );
      } finally {
        subscription.cancel();
      }

      expect(CallbackList.instance.count(), before);

      // The mapping answers into a registration that is already gone, and the suite waits for it rather than leaving it running past the test.
      await Future<void>.delayed(const Duration(milliseconds: 200));

      expect(CallbackList.instance.count(), before);
    });

    test('Call Sync Answers Empty When Nothing Owns The Function', () {
      final before = CallbackList.instance.count();

      expect(Client.callSync<String>(Request("not.found")), null);
      expect(CallbackList.instance.count(), before);
    });

    test('Call Sync Answers Nothing For A Mismatched Type', () {
      // Asking for a type the answer cannot hold is answered with nothing rather than a value read the wrong way.

      var request = Request("sample.login", [
        Param("username", "paulo"),
        Param("password", "123456"),
        Param("remember", true),
      ]);

      expect(Client.callSync<bool>(request), null);
    });

    test('Call Sync Answers The Document From A String', () {
      var request =
          '{"f":"sample.login","p":[{"n":"username","v":"paulo"},'
          '{"n":"password","v":"123456"},{"n":"remember","v":true}]}';

      expect(
        Client.callSyncFromString(request),
        '{"r":"LOGGED-WITH-REMEMBER"}',
      );
    });

    test(
      'Call Sync Answers Empty From A String When The Mapping Defers',
      () async {
        MappingList.instance.add(
          "platform.deferred.answer",
          MappingItem(deferredAnswer),
        );

        final before = CallbackList.instance.count();

        expect(
          Client.callSyncFromString('{"f":"platform.deferred.answer","p":[]}'),
          "",
        );
        expect(CallbackList.instance.count(), before);

        await Future<void>.delayed(const Duration(milliseconds: 200));
      },
    );

    test(
      'Call Sync Answers Empty From A String When Nothing Owns The Function',
      () {
        final before = CallbackList.instance.count();

        expect(Client.callSyncFromString('{"f":"not.found","p":[]}'), "");
        expect(CallbackList.instance.count(), before);
      },
    );

    test('Call Sync Answers What The Nested Mapping Produced', () {
      MappingList.instance.add(
        "platform.reverse.response",
        MappingItem(reverse),
      );

      expect(
        Client.callSync<String>(Request("sample.reverse")),
        "response-is-ok",
      );
    });
    test('A Request That Cannot Be Read Is Reported Once', () {
      // One event is one line, so the decoder says what failed and nothing repeats it.

      final records = <LogRecord>[];

      hierarchicalLoggingEnabled = true;
      Logger('XPLPC').level = Level.ALL;

      final subscription = Logger('XPLPC').onRecord.listen(records.add);

      try {
        for (final data in ["not-a-json", "null"]) {
          records.clear();

          final request = XPLPC.instance.config.serializer.decodeRequest(data);

          expect(request, null, reason: data);
          expect(
            records.where((r) => r.level >= Level.SEVERE).length,
            1,
            reason: data,
          );
        }
      } finally {
        subscription.cancel();
      }
    });
    test('A Name The Host No Longer Owns Is Answered', () {
      // The native side routes from the names the host declared, so a name dropped between the routing and the call lands here.

      PlatformProxy.addMapping("ghost.name");

      String? answer;

      Client.callFromString('{"f":"ghost.name","p":[]}', (r) => answer = r);

      expect(answer, "");
      expect(CallbackList.instance.count(), 0);
    });
  });
}
