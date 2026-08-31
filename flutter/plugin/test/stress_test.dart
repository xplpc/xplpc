import 'package:test/test.dart';
import 'package:xplpc/client/client.dart';
import 'package:xplpc/core/config.dart';
import 'package:xplpc/core/xplpc.dart';
import 'package:xplpc/data/callback_list.dart';
import 'package:xplpc/data/mapping_list.dart';
import 'package:xplpc/map/mapping_item.dart';
import 'package:xplpc/message/message.dart';
import 'package:xplpc/message/param.dart';
import 'package:xplpc/message/request.dart';
import 'package:xplpc/serializer/json_serializer.dart';

const iterationCount = 2000;

void echo(Message m, dynamic r) {
  r(m.get<String>("value"));
}

void main() {
  group('Testing Stress', () {
    setUpAll(() {
      XPLPC.instance.initialize(Config(JsonSerializer()));
    });

    setUp(() {
      // Each test starts from a clean registry, so the order they run in cannot change the result.

      CallbackList.instance.clear();
      MappingList.instance.clear();
    });

    tearDown(() {
      CallbackList.instance.clear();
      MappingList.instance.clear();
    });

    test('Callback List Never Leaks An Entry', () {
      var executed = 0;

      for (var i = 0; i < iterationCount; i++) {
        final key = "stress-$i";

        CallbackList.instance.add(key, (_) => executed++);
        CallbackList.instance.execute(key, "data");
      }

      expect(executed, iterationCount);
      expect(CallbackList.instance.count(), 0);
    });

    test('Callback List Answers Unknown Keys Without Growing', () {
      for (var i = 0; i < iterationCount; i++) {
        CallbackList.instance.execute("missing-$i", "");
      }

      expect(CallbackList.instance.count(), 0);
    });

    test('Client Answers Every Call And Leaves Nothing Pending', () {
      MappingList.instance.add("stress.echo", MappingItem(echo));

      var answered = 0;

      for (var i = 0; i < iterationCount; i++) {
        final request = Request("stress.echo", [Param("value", "stress")]);

        Client.call<String>(request, (response) {
          if (response == "stress") {
            answered++;
          }
        });
      }

      expect(answered, iterationCount);
      expect(CallbackList.instance.count(), 0);
    });

    test('Async Calls Resolve Their Own Key', () async {
      // An interleaved async call must still resolve its own key and not another.

      MappingList.instance.add("stress.echo", MappingItem(echo));

      final responses = await Future.wait(
        List.generate(
          200,
          (i) => Client.callAsync<String>(
            Request("stress.echo", [Param("value", "value-$i")]),
          ),
        ),
      );

      for (var i = 0; i < responses.length; i++) {
        expect(responses[i], "value-$i");
      }

      expect(CallbackList.instance.count(), 0);
    });
  });
}
