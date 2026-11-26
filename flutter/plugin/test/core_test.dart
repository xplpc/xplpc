import 'package:test/test.dart';
import 'package:xplpc/client/client.dart';
import 'package:xplpc/core/config.dart';
import 'package:xplpc/core/xplpc.dart';
import 'package:xplpc/data/mapping_list.dart';
import 'package:xplpc/layer/layer.dart';
import 'package:xplpc/map/mapping_item.dart';
import 'package:xplpc/serializer/json_serializer.dart';

void main() {
  group('Testing Core', () {
    test('A Mapping Added Before Initialize Is Not Recorded', () {
      // A name held here and unknown to the router is unreachable for the life of the process, so a declaration that fails takes it back.

      expect(
        () => MappingList.instance.add(
          "sample.early",
          MappingItem((m, r) => r("")),
        ),
        throwsA(
          isA<StateError>().having(
            (error) => error.message,
            "message",
            "XPLPC was not initialized",
          ),
        ),
      );

      expect(MappingList.instance.has("sample.early"), false);
      expect(MappingList.instance.count(), 0);
    });

    test('A Call Made Before Initialize Is Answered Once', () {
      final answers = <String>[];

      Client.callFromString('{"f":"sample.login","p":[]}', answers.add);

      expect(answers, [""]);
    });

    test('Initialize', () {
      expect(XPLPC.instance.initialized, false);

      XPLPC.instance.initialize(Config(JsonSerializer()));

      expect(XPLPC.instance.initialized, true);

      // A call that could not reach the native side before this point has nothing left to wait for.
      expect(Layer.pending, 0);
    });
  });
}
