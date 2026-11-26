import 'dart:async';
import 'dart:ffi' as ffi;
import 'dart:io';
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
import 'package:xplpc/message/decoded_request.dart';
import 'package:xplpc/message/message.dart';
import 'package:xplpc/message/param.dart';
import 'package:xplpc/message/request.dart';
import 'package:xplpc/message/response.dart';
import 'package:xplpc/serializer/json_serializer.dart';
import 'package:xplpc/type/dataview.dart';
import 'package:xplpc/type/typedefs.dart';

import 'client_test.reflectable.dart';

void batteryLevel(Message m, Response r) {
  final suffix = m.get<String>("suffix");
  r("100$suffix");
}

void deferredAnswer(Message m, Response r) {
  Future.delayed(const Duration(milliseconds: 50), () => r("deferred"));
}

// Answers later and says when it did, so a test can wait for an answer nobody is waiting on any more.
Target deferredSignalling(Completer<void> responded) {
  return (Message m, Response r) {
    Future.delayed(const Duration(milliseconds: 50), () {
      r("deferred");
      responded.complete();
    });
  };
}

void mismatchedType(Message m, Response r) {
  final asString = m.get<String>("value");
  final asNumber = m.get<num>("value");

  r("${asString ?? "nil"}/${asNumber?.toInt() ?? -1}");
}

void reverse(Message m, Response r) {
  r("ok");
}

// The grayscale mapping turns each opaque pixel into the average of its channels and leaves a transparent one alone.
Uint8List rgbaPixels() {
  const red = [255, 0, 0, 255];
  const green = [0, 255, 0, 255];
  const blue = [0, 0, 255, 255];
  const transparent = [0, 0, 0, 0];

  return Uint8List.fromList([...red, ...green, ...blue, ...transparent]);
}

void expectRgba(Uint8List data) {
  expect(data, rgbaPixels());
}

void expectGrayscale(Uint8List data) {
  expect(data, [85, 85, 85, 255, 85, 85, 85, 255, 85, 85, 85, 255, 0, 0, 0, 0]);
}

// Reads the view it received through the helper every consumer reads one with, which is what a mapping of this bridge does with a buffer.
void viewSum(Message m, Response r) {
  final view = m.get<DataView>("dataView");

  if (view == null) {
    r(-1);
    return;
  }

  r(ByteArrayHelper.createFromDataView(view).fold<int>(0, (a, b) => a + b));
}

Request loginRequest() {
  return Request("sample.login", [
    Param("username", "paulo"),
    Param("password", "123456"),
    Param("remember", true),
  ]);
}

// A serializer is code this library may not have written, so these two fail where a consumer's own could.
class RaisingDecoder extends JsonSerializer {
  @override
  DecodedRequest? decodeRequest(String data) =>
      throw StateError("the request could not be read");
}

class RaisingEncoder extends JsonSerializer {
  @override
  String encodeFunctionReturnValue(dynamic data) =>
      throw StateError("the answer could not be written");
}

class RaisingRequestEncoder extends JsonSerializer {
  @override
  String encodeRequest(String functionName, [List<Param>? params]) =>
      throw StateError("the request could not be written");
}

class RaisingAnswerDecoder extends JsonSerializer {
  @override
  T? decodeFunctionReturnValue<T>(String data) =>
      throw StateError("the answer could not be read");
}

// Records every line at the level a release build keeps, for as long as the body runs.
Future<List<LogRecord>> recordFailures(FutureOr<void> Function() body) async {
  final records = <LogRecord>[];

  hierarchicalLoggingEnabled = true;
  Logger('XPLPC').level = Level.ALL;

  final subscription = Logger('XPLPC').onRecord
      .where((r) => r.level >= Level.SEVERE)
      .listen(records.add);

  try {
    await body();
  } finally {
    await subscription.cancel();
  }

  return records;
}

void verifyCall<T>(Request request, void Function(dynamic) block) {
  // An assertion that fails inside a callback cannot cross the native frame that invoked it, so it is captured and raised here.

  Object? failure;
  var answers = 0;

  Client.call<T>(request, (response) {
    answers++;

    try {
      block(response);
    } catch (e) {
      failure = e;
    }
  });

  expect(answers, 1, reason: "the callback has to run exactly once");

  if (failure != null) {
    throw failure!;
  }
}

void verifyCallFromString(String requestData, void Function(String) block) {
  Object? failure;
  var answers = 0;

  Client.callFromString(requestData, (response) {
    answers++;

    try {
      block(response);
    } catch (e) {
      failure = e;
    }
  });

  expect(answers, 1, reason: "the callback has to run exactly once");

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
      // The last layer to declare a name owns it, so a test that took a name gives it back before the next one runs.
      MappingList.instance.clear();

      // A call left waiting is the defect this project has found most often, so every test is held to leaving none.
      expect(Layer.pending, 0);

      // A buffer left registered keeps memory a reader can still reach, so every test releases what it registered.
      expect(Layer.buffers, 0);
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
      expect(
        await Client.callAsync<String>(loginRequest()),
        "LOGGED-WITH-REMEMBER",
      );
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

      expect(
        await Client.callAsync<String>(Request("sample.reverse")),
        "response-is-ok",
      );
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

      final calls = List.generate(
        4,
        (_) => Client.callAsync<String>(Request("sample.async")),
      );

      // Every call was handed over before any of them answered, so all four wait at once, each counted by the router and by the token this isolate keeps for it.
      expect(Layer.pending, 8);

      final responses = await Future.wait(calls);

      expect(responses.length, 4);
    });

    test('Grayscale Image With DataView', () {
      final dataView = DataView.allocateFromByteArray(rgbaPixels());

      try {
        final request = Request("sample.image.grayscale.dataview", [
          Param("dataView", dataView),
        ]);

        verifyCall<String>(request, (response) {
          expect(response, "OK");
          expectGrayscale(ByteArrayHelper.createFromDataView(dataView));
        });
      } finally {
        dataView.dispose();
      }
    });

    test('Grayscale Image With DataView Async', () async {
      final dataView = DataView.allocateFromByteArray(rgbaPixels());

      try {
        final response = await Client.callAsync<String>(
          Request("sample.image.grayscale.dataview", [
            Param("dataView", dataView),
          ]),
        );

        expect(response, "OK");
        expectGrayscale(ByteArrayHelper.createFromDataView(dataView));
      } finally {
        dataView.dispose();
      }
    });

    test('An Answer That Is Not Utf8 Is Answered Empty', () async {
      // The bytes an answer carries were handed to an exported function by native code this library did not write, so they are read with a guard.

      MappingList.instance.add("platform.held", MappingItem((m, r) {}));

      final received = Completer<String>();
      final pending = Layer.call(
        '{"f":"platform.held","p":[]}',
        received.complete,
      );

      expect(pending.answered, false);

      final library = ffi.DynamicLibrary.open(
        Platform.environment["XPLPC_LIBRARY_PATH"]!,
      );
      final answer = library.lookupFunction<NativeAnswerFunc, AnswerFunc>(
        'xplpc_answer',
      );

      final records = await recordFailures(() async {
        final bytes = calloc<ffi.Uint8>(2);

        try {
          bytes[0] = 0xff;
          bytes[1] = 0xfe;
          answer(pending.call, bytes.cast<Utf8>(), 2);
        } finally {
          calloc.free(bytes);
        }

        expect(await received.future, "");
      });

      expect(records.single.message, contains("Unable to decode data"));
    });

    test('A Name Carrying A Nul Is Declared Whole', () {
      // The native side reads the length it is handed, so a name with a nul in it stays a name of its own rather than taking over the name before the nul.

      MappingList.instance.add(
        "sample.login\u0000x",
        MappingItem((m, r) => r("dart")),
      );

      expect(
        Client.callSyncFromString(
          '{"f":"sample.login","p":[{"n":"username","v":"paulo"},{"n":"password","v":"123456"},{"n":"remember","v":true}]}',
        ),
        '{"r":"LOGGED-WITH-REMEMBER"}',
      );
    });

    test('A Data View With A Negative Size Is Refused', () {
      // A dart int is signed and the size crosses as unsigned, so a negative one would name nearly all of memory.

      final memory = calloc<ffi.Uint8>(16);

      try {
        final dataView = DataView.createFromByteBuffer(memory, -1);

        expect(dataView.buffer, 0);
        expect(Layer.buffers, 0);
      } finally {
        calloc.free(memory);
      }
    });

    test('A Borrowed Data View Is Modified In Place', () async {
      // The mapping walks the bytes the caller registered rather than a copy, so the caller's own memory carries the answer.

      final pixels = rgbaPixels();
      final memory = calloc<ffi.Uint8>(pixels.length);

      try {
        memory.asTypedList(pixels.length).setAll(0, pixels);

        final dataView = DataView.createFromByteBuffer(memory, pixels.length);

        try {
          final response = await Client.callAsync<String>(
            Request("sample.image.grayscale.dataview", [
              Param("dataView", dataView),
            ]),
          );

          expect(response, "OK");
          expectGrayscale(memory.asTypedList(pixels.length));
        } finally {
          expect(dataView.dispose(), true);
        }
      } finally {
        calloc.free(memory);
      }
    });

    test('A Released Data View Reaches The Mapping As Nothing', () async {
      // The handle still crosses the wire after it was released, and the table is what refuses to read it.

      final dataView = DataView.allocateFromByteArray(rgbaPixels());

      expect(dataView.dispose(), true);

      final response = await Client.callAsync<String>(
        Request("sample.image.grayscale.dataview", [
          Param("dataView", dataView),
        ]),
      );

      expect(response, "INVALID-DATA");
    });

    test('A Dart Mapping Reads A Data View It Received', () async {
      // A parameter arrives as the document the wire carried, so reading it as a view has to build one rather than cast it.

      MappingList.instance.add("platform.view.sum", MappingItem(viewSum));

      final dataView = DataView.allocateFromByteArray(
        Uint8List.fromList([1, 2, 3, 4]),
      );

      try {
        final response = await Client.callAsync<int>(
          Request("platform.view.sum", [Param("dataView", dataView)]),
        );

        expect(response, 10);
      } finally {
        dataView.dispose();
      }
    });

    test('DataView', () {
      verifyCall<DataView>(Request("sample.dataview"), (response) {
        expect(response, isA<DataView>());
        expectRgba(ByteArrayHelper.createFromDataView(response as DataView));
      });
    });

    test('DataView Async', () async {
      final response = await Client.callAsync<DataView>(
        Request("sample.dataview"),
      );

      expect(response, isNotNull);

      final original = ByteArrayHelper.createFromDataView(response!);
      expectRgba(original);

      // A view this isolate did not register is not its to release, so the owner keeps it.
      expect(response.dispose(), false);

      final copy = DataView.allocateFromByteArray(original);

      try {
        final processed = await Client.callAsync<String>(
          Request("sample.image.grayscale.dataview", [Param("dataView", copy)]),
        );

        expect(processed, "OK");
        expectGrayscale(ByteArrayHelper.createFromDataView(copy));
        expectRgba(original);
      } finally {
        copy.dispose();
      }
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
        final held = <Response>[];

        MappingList.instance.add(
          "platform.reverse.response",
          MappingItem((m, r) => held.add(r)),
        );

        final answered = Completer<String?>();

        Client.call<String>(Request("sample.reverse"), (response) {
          answered.complete(response);
        });

        expect(held.length, 1);
        expect(answered.isCompleted, false);

        held.single("ok");

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

    test('Unknown Function Answers Empty Exactly Once', () async {
      // Nothing owns this name, so the router answers the caller itself and nothing is left to answer it again.

      final answers = <String>[];

      Client.callFromString('{"f":"not.found","p":[]}', answers.add);

      expect(answers, [""]);
      expect(await Client.callAsync<String>(Request("not.found")), null);

      await Future<void>.delayed(const Duration(milliseconds: 50));

      expect(answers, [""]);
    });

    test('Call Async Gives The Event Loop A Turn Before Answering', () async {
      // A call site has to behave the same whether the mapping is quick or slow, so the loop is reached even when the answer is already there.

      final order = <String>[];

      scheduleMicrotask(() => order.add("loop"));

      await Client.callAsync<String>(Request("sample.version"));
      order.add("after await");

      expect(order, ["loop", "after await"]);
    });

    test('A Call Reaching The Layer Before Initialize Is Answered Empty', () {
      // The router can hand a call over while the isolate is not ready, so being reached then answers rather than hangs.

      MappingList.instance.add(
        "platform.battery.level",
        MappingItem(batteryLevel),
      );

      XPLPC.instance.initialized = false;

      try {
        final pending = Layer.call(
          '{"f":"platform.battery.level","p":[{"n":"suffix","v":"%"}]}',
          (_) {},
        );

        expect(pending.answer, "");
      } finally {
        XPLPC.instance.initialized = true;
      }
    });

    test(
      'A Call Made Before Initialize Says So Once And Answers Empty',
      () async {
        // A caller that forgot to bring the library up learns that from one line rather than from what failed after it.

        XPLPC.instance.initialized = false;

        try {
          final answered = <String?>[];
          final raw = <String>[];

          final records = await recordFailures(() async {
            Client.call<String>(Request("sample.login"), answered.add);
            Client.callFromString('{"f":"sample.login","p":[]}', raw.add);

            expect(Client.callSync<String>(Request("sample.login")), isNull);
            expect(
              Client.callSyncFromString('{"f":"sample.login","p":[]}'),
              "",
            );
            expect(
              await Client.callAsync<String>(Request("sample.login")),
              isNull,
            );
            expect(
              await Client.callAsyncFromString('{"f":"sample.login","p":[]}'),
              "",
            );
          });

          expect(answered, [null]);
          expect(raw, [""]);
          expect(records.length, 6);
          expect(
            records.every(
              (record) => record.message.contains("XPLPC was not initialized"),
            ),
            isTrue,
          );
        } finally {
          XPLPC.instance.initialized = true;
        }
      },
    );

    test('Call Sync Answers The Value From An Inline Mapping', () {
      // A mapping that answers before it returns has already resolved the call, so the value is there to be read.

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
      // A mapping that answers later cannot be read synchronously, so the call is cancelled and the answer it produces afterwards changes nothing.

      final responded = Completer<void>();

      MappingList.instance.add(
        "platform.deferred.answer",
        MappingItem(deferredSignalling(responded)),
      );

      final failures = await recordFailures(() async {
        expect(
          Client.callSync<String>(Request("platform.deferred.answer")),
          null,
        );

        await responded.future;
        await Future<void>.delayed(const Duration(milliseconds: 50));
      });

      expect(failures.length, 1);
      expect(failures.single.message, contains("did not answer synchronously"));
      expect(Client.callSync<String>(loginRequest()), "LOGGED-WITH-REMEMBER");
    });

    test('Call Sync Answers Empty When A Core Mapping Defers', () async {
      // The core mapping answers from a thread of its own after the call was cancelled, so nothing reaches the isolate.

      final failures = await recordFailures(() async {
        expect(Client.callSync<String>(Request("sample.async")), null);

        await Future<void>.delayed(const Duration(milliseconds: 1500));
      });

      expect(failures.length, 1);
      expect(failures.single.message, contains("did not answer synchronously"));
    });

    test('Call Sync Answers Empty When Nothing Owns The Function', () {
      expect(Client.callSync<String>(Request("not.found")), null);
    });

    test('Call Sync Answers Nothing For A Mismatched Type', () {
      // Asking for a type the answer cannot hold is answered with nothing rather than a value read the wrong way.

      expect(Client.callSync<bool>(loginRequest()), null);
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
        final responded = Completer<void>();

        MappingList.instance.add(
          "platform.deferred.answer",
          MappingItem(deferredSignalling(responded)),
        );

        final failures = await recordFailures(() async {
          expect(
            Client.callSyncFromString(
              '{"f":"platform.deferred.answer","p":[]}',
            ),
            "",
          );

          await responded.future;
          await Future<void>.delayed(const Duration(milliseconds: 50));
        });

        expect(failures.length, 1);
      },
    );

    test(
      'Call Sync Answers Empty From A String When Nothing Owns The Function',
      () {
        expect(Client.callSyncFromString('{"f":"not.found","p":[]}'), "");
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

    test('A Serializer That Raises Still Answers The Caller', () async {
      // The answer is written later than the call, where nothing above the layer can catch what the serializer raises.

      final config = XPLPC.instance.config;

      MappingList.instance.add(
        "platform.serializer.raises",
        MappingItem(deferredAnswer),
      );

      try {
        XPLPC.instance.config = Config(RaisingDecoder());

        final decoded = await recordFailures(() {
          expect(
            Client.callSyncFromString(
              '{"f":"platform.serializer.raises","p":[]}',
            ),
            "",
          );
        });

        expect(decoded.single.message, contains("Error when decode request"));

        XPLPC.instance.config = Config(RaisingEncoder());

        final encoded = await recordFailures(() async {
          expect(
            await Client.callAsyncFromString(
              '{"f":"platform.serializer.raises","p":[]}',
            ),
            "",
          );
        });

        expect(
          encoded.single.message,
          contains("Error when encode the answer"),
        );
      } finally {
        XPLPC.instance.config = config;
      }
    });

    test(
      'A Serializer That Raises Writing The Request Answers Every Entry Point',
      () async {
        // A serializer of the caller's own can raise while it writes the request, and every typed entry point answers rather than handing the exception over.

        final config = XPLPC.instance.config;

        try {
          XPLPC.instance.config = Config(RaisingRequestEncoder());

          final answered = <String?>[];

          final records = await recordFailures(() async {
            Client.call<String>(Request("sample.login"), answered.add);

            expect(Client.callSync<String>(Request("sample.login")), isNull);
            expect(
              await Client.callAsync<String>(Request("sample.login")),
              isNull,
            );
          });

          expect(answered, [null]);
          expect(records.length, 3);
          expect(
            records.every(
              (record) =>
                  record.message.contains("Error when encode the request"),
            ),
            isTrue,
          );
        } finally {
          XPLPC.instance.config = config;
        }
      },
    );

    test(
      'A Serializer That Raises Reading A Late Answer Still Answers The Caller',
      () async {
        // The answer is decoded on the event loop turn that delivers it, where nothing above the client can catch what the serializer raises.

        final config = XPLPC.instance.config;

        MappingList.instance.add(
          "platform.serializer.raises",
          MappingItem(deferredAnswer),
        );

        try {
          XPLPC.instance.config = Config(RaisingAnswerDecoder());

          final records = await recordFailures(() async {
            final value = await Client.callAsync<String>(
              Request("platform.serializer.raises"),
            ).timeout(const Duration(seconds: 5));

            expect(value, isNull);
          });

          expect(records.single.message, contains("Error when decode data"));
        } finally {
          XPLPC.instance.config = config;
        }
      },
    );

    test('A Callback That Raises On A Late Answer Is Reported', () async {
      // A late answer is delivered by the listener the native side posts to, where nothing above the callback could catch it.

      MappingList.instance.add(
        "platform.late.raises",
        MappingItem(deferredAnswer),
      );

      final answered = Completer<void>();

      final records = await recordFailures(() async {
        Client.call<String>(Request("platform.late.raises"), (_) {
          answered.complete();
          throw StateError("the callback failed");
        });

        await answered.future.timeout(const Duration(seconds: 5));
        await Future<void>.delayed(Duration.zero);
      });

      expect(
        records.single.message,
        contains("The callback raised while being answered"),
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

    test(
      'A Response Without A Value Fills No Reflected Type And Reports Nothing',
      () {
        // A mapping with nothing to answer travels as null, which is the documented answer rather than a document that failed.

        final serializer = XPLPC.instance.config.serializer;
        final records = <LogRecord>[];

        hierarchicalLoggingEnabled = true;
        Logger('XPLPC').level = Level.ALL;

        final subscription = Logger('XPLPC').onRecord.listen(records.add);

        try {
          expect(
            serializer.decodeFunctionReturnValue<DataView>('{"r":null}'),
            null,
          );
          expect(serializer.decodeFunctionReturnValue<DataView>('{}'), null);
          expect(records.where((r) => r.level >= Level.SEVERE), isEmpty);

          final view = serializer.decodeFunctionReturnValue<DataView>(
            '{"r":{"buffer":128}}',
          );

          expect(view?.buffer, 128);
        } finally {
          subscription.cancel();
        }
      },
    );

    test('A Name The Host No Longer Owns Is Answered', () {
      // The router routes from the names the isolate declared, so a name it declared without a mapping behind it lands here.

      Layer.declare("ghost.name");

      try {
        verifyCallFromString('{"f":"ghost.name","p":[]}', (response) {
          expect(response, "");
        });
      } finally {
        Layer.retract("ghost.name");
      }
    });

    test('A Dart Mapping Declared Under A Core Name Takes It Over', () {
      // The last layer to declare a name owns it, and clearing gives it back to the layer that declared it before.

      MappingList.instance.add(
        "sample.login",
        MappingItem((m, r) => r("FROM-DART-${m.get<String>("username")}")),
      );

      expect(Client.callSync<String>(loginRequest()), "FROM-DART-paulo");

      verifyCall<String>(loginRequest(), (response) {
        expect(response, "FROM-DART-paulo");
      });

      MappingList.instance.clear();

      expect(Client.callSync<String>(loginRequest()), "LOGGED-WITH-REMEMBER");
    });

    test('A Cancelled Call Is Never Answered', () async {
      // Cancelling takes the call back from the router, so the answer the mapping produces afterwards reaches nobody.

      final responded = Completer<void>();

      MappingList.instance.add(
        "platform.deferred.answer",
        MappingItem(deferredSignalling(responded)),
      );

      final answers = <String>[];
      final pending = Layer.call(
        '{"f":"platform.deferred.answer","p":[]}',
        answers.add,
      );

      expect(pending.answered, false);
      expect(Layer.cancel(pending), true);
      expect(Layer.cancel(pending), false);

      final failures = await recordFailures(() async {
        await responded.future;
        await Future<void>.delayed(const Duration(milliseconds: 50));
      });

      expect(answers, isEmpty);
      expect(failures, isEmpty);
    });

    test('A Callback That Raises On An Inline Answer Runs Once', () {
      // The callback is the caller's own code, so what it raises reaches the caller rather than being taken for a native side that could not be reached.

      var answers = 0;

      expect(
        () => Client.call<String>(loginRequest(), (response) {
          answers++;
          throw StateError("raised by the callback");
        }),
        throwsStateError,
      );

      expect(answers, 1);
    });

    test('A Call Answered Inline Cannot Be Cancelled', () {
      final answers = <String>[];
      final pending = Layer.call(loginRequest().data(), answers.add);

      expect(pending.answered, true);
      expect(pending.answer, '{"r":"LOGGED-WITH-REMEMBER"}');
      expect(answers, isEmpty);
      expect(Layer.cancel(pending), false);
    });
  });
}
