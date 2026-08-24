import 'dart:convert';

import 'package:logging/logging.dart';
import 'package:test/test.dart';
import 'package:xplpc/message/param.dart';
import 'package:xplpc/serializer/json_serializer.dart';
import 'package:xplpc/type/dataview.dart';

void main() {
  group('Testing Serializer', () {
    final serializer = JsonSerializer();

    dynamic encodedValue(Param param) {
      final data = serializer.encodeRequest("sample.wire", [param]);
      return jsonDecode(data)["p"][0]["v"];
    }

    test('The Wire Carries No Insignificant Whitespace', () {
      // The reference format is what the c++ core writes, and a serializer that spaces its output carries different bytes for the same value.

      final data = serializer.encodeRequest("sample.wire", [
        Param("a", 1),
        Param("b", "x"),
      ]);

      expect(
        data,
        '{"f":"sample.wire","p":[{"n":"a","v":1},{"n":"b","v":"x"}]}',
      );
    });

    test('A Request Without Parameters Carries An Empty Array', () {
      // The format says p is an array, and writing null there is a request no bridge can read.

      final data = serializer.encodeRequest("sample.wire");

      expect(jsonDecode(data)["p"], isEmpty);
      expect(jsonDecode(data)["p"], isA<List<dynamic>>());
    });

    test('A Request Without Parameters Decodes To An Empty Message', () {
      // The reference reads a missing parameter array as no parameters rather than as a broken request.

      expect(
        serializer.decodeRequest('{"f":"sample.wire","p":null}'),
        isNotNull,
      );
      expect(serializer.decodeRequest('{"f":"sample.wire"}'), isNotNull);
      expect(serializer.decodeRequest('{"f":"sample.wire","p":[]}'), isNotNull);
    });

    test('Data View Travels As Pointer And Size', () {
      // The reference format is what the c++ core writes, and every bridge has to agree with it.

      final value = encodedValue(Param("dataView", DataView(128, 4)));

      expect(value["ptr"], 128);
      expect(value["size"], 4);
    });

    test('Unsupported Value Answers Empty', () {
      // This bridge carries no character and no date type, so a value it cannot represent is reported instead of written wrong.

      final serializer = JsonSerializer();

      expect(
        serializer.encodeRequest("f", [Param("date", DateTime(1985, 9, 10))]),
        "",
      );
    });

    test('Integer Keeps Its Precision', () {
      expect(encodedValue(Param("int64", 9007199254740993)), 9007199254740993);
    });

    test('Null Value Is Preserved', () {
      expect(encodedValue(Param("value", null)), null);
    });

    test('Invalid Data Answers Empty', () {
      // Every bridge answers the empty value for its type when the data cannot be read.

      final serializer = JsonSerializer();

      expect(serializer.decodeRequest("not-a-json"), null);
      expect(serializer.decodeRequest("{}")?.functionName, "");
      expect(serializer.decodeFunctionReturnValue<String>("not-a-json"), null);
    });

    test('An Empty Response Is Not A Failure', () {
      // The empty string is what every failing path answers with, so handing it to the parser would report a failure that did not happen.

      final serializer = JsonSerializer();
      final records = <LogRecord>[];

      hierarchicalLoggingEnabled = true;
      Logger('XPLPC').level = Level.ALL;

      final subscription = Logger('XPLPC').onRecord.listen(records.add);

      try {
        expect(serializer.decodeFunctionReturnValue<String>(""), null);
        expect(records, isEmpty);

        expect(
          serializer.decodeFunctionReturnValue<String>("not-a-json"),
          null,
        );
        expect(records, isNotEmpty);
      } finally {
        subscription.cancel();
      }
    });
    test('A Number That Is Not Finite Is Refused', () {
      // The wire has no token for infinity, so writing one would put a document on it that no other bridge can read.

      final serializer = JsonSerializer();

      expect(serializer.encodeFunctionReturnValue(double.infinity), "");
      expect(serializer.encodeFunctionReturnValue(double.nan), "");
      expect(serializer.encodeFunctionReturnValue(2.5), '{"r":2.5}');
    });
  });
}
