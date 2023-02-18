import 'package:test/test.dart';
import 'package:xplpc/core/config.dart';
import 'package:xplpc/core/xplpc.dart';
import 'package:xplpc/serializer/json_serializer.dart';

void main() {
  group('Testing Core', () {
    test('Initialize', () {
      expect(XPLPC.instance.initialized, false);

      XPLPC.instance.initialize(
        Config(
          JsonSerializer(),
        ),
      );

      expect(XPLPC.instance.initialized, true);
    });
  });
}
