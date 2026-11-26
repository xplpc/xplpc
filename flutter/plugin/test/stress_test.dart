import 'package:test/test.dart';
import 'package:xplpc/client/client.dart';
import 'package:xplpc/core/config.dart';
import 'package:xplpc/core/xplpc.dart';
import 'package:xplpc/data/mapping_list.dart';
import 'package:xplpc/layer/layer.dart';
import 'package:xplpc/map/mapping_item.dart';
import 'package:xplpc/message/message.dart';
import 'package:xplpc/message/param.dart';
import 'package:xplpc/message/request.dart';
import 'package:xplpc/message/response.dart';
import 'package:xplpc/serializer/json_serializer.dart';

const iterationCount = 2000;

void echo(Message m, Response r) {
  r(m.get<String>("value"));
}

// Answers in the reverse order the calls arrived, so a call resolved by the wrong answer reads another call's value.
void reversedEcho(Message m, Response r) {
  final index = m.get<int>("index") ?? 0;

  Future.delayed(Duration(milliseconds: 200 - index), () {
    r(m.get<String>("value"));
  });
}

void main() {
  group('Testing Stress', () {
    setUpAll(() {
      XPLPC.instance.initialize(Config(JsonSerializer()));
    });

    setUp(() {
      // Each test starts from a clean registry, so the order they run in cannot change the result.

      MappingList.instance.clear();
    });

    tearDown(() {
      MappingList.instance.clear();

      expect(Layer.pending, 0);
    });

    test('Client Answers Every Call Exactly Once', () {
      MappingList.instance.add("stress.echo", MappingItem(echo));

      final answers = List.filled(iterationCount, 0);

      for (var i = 0; i < iterationCount; i++) {
        final request = Request("stress.echo", [Param("value", "stress-$i")]);

        Client.call<String>(request, (response) {
          if (response == "stress-$i") {
            answers[i]++;
          }
        });
      }

      expect(answers.every((count) => count == 1), true);
    });

    test('Unknown Names Are Each Answered Once With Nothing', () async {
      // Every one of these writes a native error line, so the count is kept to what proves the point.

      final answers = <String>[];

      for (var i = 0; i < 200; i++) {
        Client.callFromString('{"f":"missing-$i","p":[]}', answers.add);
      }

      await Future<void>.delayed(const Duration(milliseconds: 50));

      expect(answers.length, 200);
      expect(answers.every((answer) => answer.isEmpty), true);
    });

    test('Async Calls Resolve Their Own Call', () async {
      // An interleaved async call must still resolve its own call and not another.

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
    });

    test('Late Answers Out Of Order Resolve Their Own Call', () async {
      // Every answer arrives after its call returned and in the reverse order, so each one has to find its caller by the call it belongs to.

      MappingList.instance.add("stress.reversed", MappingItem(reversedEcho));

      final responses = await Future.wait(
        List.generate(
          200,
          (i) => Client.callAsync<String>(
            Request("stress.reversed", [
              Param("index", i),
              Param("value", "value-$i"),
            ]),
          ),
        ),
      );

      for (var i = 0; i < responses.length; i++) {
        expect(responses[i], "value-$i");
      }
    });

    test('Core Mapping Answers From Its Own Thread To Every Caller', () async {
      // Each answer is produced on a thread the isolate does not own, so every one of them reaches it through its event loop.

      final responses = await Future.wait(
        List.generate(
          32,
          (_) => Client.callAsyncFromString('{"f":"sample.async","p":[]}'),
        ),
      );

      expect(responses.length, 32);
      expect(responses.every((r) => r == '{"r":null}'), true);
    });
  });
}
