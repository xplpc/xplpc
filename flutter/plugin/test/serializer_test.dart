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

    test('Data View Travels As A Handle', () {
      // The reference format is what the c++ core writes, and a view carries the handle its owner registered and never an address.

      final data = serializer.encodeRequest("sample.wire", [
        Param("dataView", DataView(128)),
      ]);

      expect(
        data,
        '{"f":"sample.wire","p":[{"n":"dataView","v":{"buffer":128}}]}',
      );
    });

    test('Unsupported Value Answers Empty', () {
      // This bridge carries no character and no date type, so a value it cannot represent is reported instead of written wrong.

      expect(
        serializer.encodeRequest("f", [Param("date", DateTime(1985, 9, 10))]),
        "",
      );
    });

    test('Integer Keeps Its Precision', () {
      expect(encodedValue(Param("int64", 9007199254740993)), 9007199254740993);
    });

    test('An Empty Response Is Not A Failure', () {
      // The empty string is what every failing path answers with, so handing it to the parser would report a failure that did not happen.

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

    test('A Refused Request Names The Function And Not The Values', () {
      // The conformance cases say these documents are refused, and what is left to pin is that the one line each writes carries no value the caller sent.

      final records = <LogRecord>[];

      hierarchicalLoggingEnabled = true;
      Logger('XPLPC').level = Level.ALL;

      final subscription = Logger('XPLPC').onRecord.listen(records.add);

      try {
        for (final data in [
          '{"f":"auth","p":[{"n":"token","v":"allow"},{"n":"token","v":"deny"}]}',
          '{"f":"auth","p":[{"n":"token","v":null},{"n":"token","v":"deny"}]}',
          '{"f":"auth","p":[{"n":1,"v":"allow"}]}',
          '{"f":"auth","p":[{"v":"allow"}]}',
          '{"f":"auth","p":["token"]}',
        ]) {
          records.clear();

          expect(serializer.decodeRequest(data), null, reason: data);

          final failure = records.singleWhere((r) => r.level >= Level.SEVERE);

          expect(failure.message, contains('"auth"'), reason: data);
          expect(failure.message, isNot(contains('allow')));
          expect(failure.message, isNot(contains('deny')));
        }
      } finally {
        subscription.cancel();
      }
    });

    test('A Program Without Reflection Reads Plain Values Silently', () {
      // Reflection is generated per program and this one never initialized it, which is a consumer reading plain values rather than a failure.

      final records = <LogRecord>[];

      hierarchicalLoggingEnabled = true;
      Logger('XPLPC').level = Level.ALL;

      final subscription = Logger('XPLPC').onRecord.listen(records.add);

      try {
        expect(serializer.decodeFunctionReturnValue<String>('{"r":"x"}'), "x");
        expect(serializer.decodeFunctionReturnValue<int>('{"r":7}'), 7);
        expect(records.where((r) => r.level >= Level.SEVERE), isEmpty);
      } finally {
        subscription.cancel();
      }
    });

    test(
      'An Answer Of Another Type Is Reported As Every Bridge Reports It',
      () {
        // A document that parsed with a value the type cannot hold is not a parse failure, and one that carries no value is not a failure at all.

        final records = <LogRecord>[];

        hierarchicalLoggingEnabled = true;
        Logger('XPLPC').level = Level.ALL;

        final subscription = Logger('XPLPC').onRecord.listen(records.add);

        try {
          expect(
            serializer.decodeFunctionReturnValue<int>('{"r":"text"}'),
            null,
          );
          expect(
            records.singleWhere((r) => r.level >= Level.SEVERE).message,
            contains('Value has another type'),
          );

          records.clear();

          expect(serializer.decodeFunctionReturnValue<int>('[1]'), null);
          expect(serializer.decodeFunctionReturnValue<int>('42'), null);
          expect(records.where((r) => r.level >= Level.SEVERE), isEmpty);
        } finally {
          subscription.cancel();
        }
      },
    );

    test('A Refused Parameter Is Reported As Every Bridge Reports It', () {
      final records = <LogRecord>[];

      hierarchicalLoggingEnabled = true;
      Logger('XPLPC').level = Level.ALL;

      final subscription = Logger('XPLPC').onRecord.listen(records.add);

      try {
        for (final (data, failure) in [
          (
            '{"f":"auth","p":[{"n":"token","v":1},{"n":"token","v":2}]}',
            'Parameter "token" of function "auth" appears more than once',
          ),
          (
            '{"f":"auth","p":[{"n":1,"v":2}]}',
            'Function "auth" carries a parameter whose name is not text',
          ),
          (
            '{"f":"auth","p":["token"]}',
            'Function "auth" carries a parameter whose name is not text',
          ),
        ]) {
          records.clear();

          expect(serializer.decodeRequest(data), null, reason: data);
          expect(
            records.singleWhere((r) => r.level >= Level.SEVERE).message,
            contains(failure),
            reason: data,
          );
        }
      } finally {
        subscription.cancel();
      }
    });

    test('A Parameter Carrying No Value Is Read As Nothing Silently', () {
      // The format lets a parameter arrive with its name and no value, which is not a failure to report.

      final records = <LogRecord>[];

      hierarchicalLoggingEnabled = true;
      Logger('XPLPC').level = Level.ALL;

      final subscription = Logger('XPLPC').onRecord.listen(records.add);

      try {
        final request = serializer.decodeRequest(
          '{"f":"sample.empty","p":[{"n":"a","v":null},{"n":"b"}]}',
        );

        expect(request?.message.get<String>("a"), null);
        expect(request?.message.get<String>("b"), null);
        expect(records.where((r) => r.level >= Level.SEVERE), isEmpty);
      } finally {
        subscription.cancel();
      }
    });

    test('An Integer Is Read As A Double Only When The Double Holds It', () {
      // The largest integer rounds up to a double that is not that integer, so reading it as one is a mismatch reported like any other.

      final records = <LogRecord>[];

      hierarchicalLoggingEnabled = true;
      Logger('XPLPC').level = Level.ALL;

      final subscription = Logger('XPLPC').onRecord.listen(records.add);

      try {
        expect(
          serializer.decodeFunctionReturnValue<double>(
            '{"r":9223372036854775807}',
          ),
          null,
        );
        expect(records.where((r) => r.level >= Level.SEVERE).length, 1);

        expect(
          serializer.decodeFunctionReturnValue<double>(
            '{"r":9007199254740992}',
          ),
          9007199254740992.0,
        );
      } finally {
        subscription.cancel();
      }
    });

    test('A Number Past A Double Is Refused Wherever It Sits In An Answer', () {
      // The decoder reads such a number as infinity at any depth, and another host in the process can answer one.

      expect(
        serializer.decodeFunctionReturnValue<List<dynamic>>('{"r":[1e400]}'),
        null,
      );
      expect(
        serializer.decodeFunctionReturnValue<Map<String, dynamic>>(
          '{"r":{"x":[2,1e400]}}',
        ),
        null,
      );
      expect(
        serializer.decodeFunctionReturnValue<List<dynamic>>('{"r":[1.5,2]}'),
        [1.5, 2],
      );
    });

    test('A Number That Is Not Finite Is Refused', () {
      // The wire has no token for infinity, so writing one would put a document on it that no other bridge can read.

      expect(serializer.encodeFunctionReturnValue(double.infinity), "");
      expect(serializer.encodeFunctionReturnValue(double.nan), "");
      expect(serializer.encodeFunctionReturnValue(2.5), '{"r":2.5}');
    });
  });
}
