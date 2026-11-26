import 'dart:convert';
import 'dart:io';
import 'dart:typed_data';

import 'package:logging/logging.dart';
import 'package:test/test.dart';
import 'package:xplpc/core/config.dart';
import 'package:xplpc/core/xplpc.dart';
import 'package:xplpc/message/message.dart';
import 'package:xplpc/message/param.dart';
import 'package:xplpc/message/request.dart';
import 'package:xplpc/serializer/json_serializer.dart';
import 'package:xplpc/type/dataview.dart';

import 'conformance_test.reflectable.dart';

// Every bridge runs the same cases, which live at the root of the repository and are listed when the suite is loaded, so a case added there needs no code here.
Directory conformanceRoot() {
  var directory = Directory.current.absolute;

  while (true) {
    final candidate = Directory("${directory.path}/conformance");

    if (File("${candidate.path}/README.md").existsSync()) {
      return candidate;
    }

    final parent = directory.parent;

    if (parent.path == directory.path) {
      throw StateError(
        "The conformance cases were not found above ${Directory.current.path}",
      );
    }

    directory = parent;
  }
}

Map<String, File> casesOf(Directory root, String group) {
  final files = Directory("${root.path}/$group")
      .listSync()
      .whereType<File>()
      .where((file) => file.path.endsWith(".json"))
      .toList();

  files.sort((a, b) => a.path.compareTo(b.path));

  final cases = <String, File>{};

  for (final file in files) {
    final name = file.uri.pathSegments.last;
    cases[name.substring(0, name.length - ".json".length)] = file;
  }

  return cases;
}

Map<String, dynamic> readCase(File file) {
  return jsonDecode(file.readAsStringSync()) as Map<String, dynamic>;
}

Param paramOf(Map<String, dynamic> param) {
  final name = param["name"] as String;
  final value = param["value"];
  final type = param["type"] as String;

  switch (type) {
    case "int":
    case "int64":
      return Param(name, value as int);
    case "double":
      return Param(name, value as double);
    case "string":
      return Param(name, value as String);
    case "bool":
      return Param(name, value as bool);
    case "null":
      return Param(name, null);
    case "dataview":
      return Param(name, DataView(value as int));
    case "list<int>":
    case "list<string>":
    case "list<list<int>>":
    case "map<int>":
    case "record":
      return Param(name, value);
    case "list<dataview>":
      return Param(
        name,
        (value as List<dynamic>)
            .map((handle) => DataView(handle as int))
            .toList(),
      );
  }

  fail("The case declares a parameter type this runner does not know: $type");
}

const notSent = {
  "uint64": "a dart int is signed and cannot hold the unsigned 64 bit range",
  "char": "dart carries no character type, so a caller sends the code point as an int",
  "date":
      "dart carries no date type, so a caller sends the milliseconds as an int",
};

String? unsentReason(Map<String, dynamic> testCase) {
  for (final param in testCase["params"] as List<dynamic>) {
    final reason = notSent[(param as Map<String, dynamic>)["type"]];

    if (reason != null) {
      return reason;
    }
  }

  return null;
}

void expectParam(Message message, String name, dynamic expected) {
  final reason = 'parameter "$name"';

  if (expected == null) {
    expect(message.get<Object?>(name), isNull, reason: reason);
    return;
  }

  if (expected is String) {
    expect(message.get<String>(name), expected, reason: reason);
    return;
  }

  if (expected is bool) {
    expect(message.get<bool>(name), expected, reason: reason);
    return;
  }

  if (expected is int) {
    expect(message.get<int>(name), expected, reason: reason);
    return;
  }

  fail(
    "The case expects a parameter value this runner does not know: $expected",
  );
}

// Dart has one integer and one floating type, so a declared width is checked on what the serializer answered, which is the width the other typed bridges declare.
int? narrowToInteger(int? value, int minimum, int maximum) {
  if (value == null || value < minimum || value > maximum) {
    return null;
  }

  return value;
}

// A fraction is rounded to the nearest float, while an integer has to arrive exactly, which is the float the other typed bridges declare.
double? narrowToFloat(JsonSerializer serializer, String document) {
  final value = serializer.decodeFunctionReturnValue<double>(document);

  if (value == null || !value.isFinite || value.abs() > 3.4028234663852886e38) {
    return null;
  }

  final written = serializer.decodeFunctionReturnValue<Object>(document);

  if (written is int && Float32List.fromList([value]).first != value) {
    return null;
  }

  return value;
}

const integerWidths = {
  "int8": (-128, 127),
  "int16": (-32768, 32767),
  "int32": (-2147483648, 2147483647),
  "uint8": (0, 255),
  "uint16": (0, 65535),
  "uint32": (0, 4294967295),
};

const notCarried = {
  "uint64": "a dart int is signed and cannot hold the unsigned 64 bit range",
  "char": "dart carries no character type",
  "date": "dart carries no date type",
  "list<int32>": "dart answers a list decoded, without an element type",
  "list<string>": "dart answers a list decoded, without an element type",
  "list<list<int32>>": "dart answers a list decoded, without an element type",
  "map<int32>": "dart answers a map decoded, without a value type",
  "todo": "dart answers a consumer type decoded as a map",
};

dynamic readValue(JsonSerializer serializer, String type, String document) {
  final width = integerWidths[type];

  if (width != null) {
    return narrowToInteger(
      serializer.decodeFunctionReturnValue<int>(document),
      width.$1,
      width.$2,
    );
  }

  switch (type) {
    case "int64":
      return serializer.decodeFunctionReturnValue<int>(document);
    case "double":
      return serializer.decodeFunctionReturnValue<double>(document);
    case "float":
      return narrowToFloat(serializer, document);
    case "bool":
      return serializer.decodeFunctionReturnValue<bool>(document);
    case "string":
      return serializer.decodeFunctionReturnValue<String>(document);
    case "dataview":
      return serializer.decodeFunctionReturnValue<DataView>(document)?.buffer;
  }

  fail("The case declares a type this runner does not know: $type");
}

void main() {
  initializeReflectable();

  final root = conformanceRoot();
  final serializer = JsonSerializer();

  setUpAll(() {
    hierarchicalLoggingEnabled = true;
    Logger('XPLPC').level = Level.ALL;

    XPLPC.instance.initialize(Config(JsonSerializer()));
  });

  group('wire', () {
    for (final entry in casesOf(root, "wire").entries) {
      final testCase = readCase(entry.value);

      test(entry.key, skip: unsentReason(testCase), () {
        final params = (testCase["params"] as List<dynamic>)
            .map((param) => paramOf(param as Map<String, dynamic>))
            .toList();
        final request = Request(testCase["function"] as String, params);

        expect(request.data(), testCase["bytes"] as String);
      });
    }
  });

  group('accept', () {
    for (final entry in casesOf(root, "accept").entries) {
      test(entry.key, () {
        final testCase = readCase(entry.value);
        final decoded = serializer.decodeRequest(
          testCase["document"] as String,
        );

        expect(
          decoded,
          isNotNull,
          reason: "the decoder refused a document every bridge reads",
        );
        expect(decoded!.functionName, testCase["function"] as String);

        final params = testCase["params"] as Map<String, dynamic>;

        for (final param in params.entries) {
          expectParam(decoded.message, param.key, param.value);
        }
      });
    }
  });

  group('refuse', () {
    for (final entry in casesOf(root, "refuse").entries) {
      test(entry.key, () {
        final testCase = readCase(entry.value);
        final records = <LogRecord>[];
        final subscription = Logger('XPLPC').onRecord.listen(records.add);

        try {
          final decoded = serializer.decodeRequest(
            testCase["document"] as String,
          );

          expect(
            decoded,
            isNull,
            reason:
                'the decoder read a document every bridge refuses, naming function "${decoded?.functionName}"',
          );
        } finally {
          subscription.cancel();
        }

        final failures = records.where((r) => r.level >= Level.SEVERE);

        expect(failures.length, 1, reason: "one refusal writes one error line");
      });
    }
  });

  group('values', () {
    for (final entry in casesOf(root, "values").entries) {
      final testCase = readCase(entry.value);
      final type = testCase["type"] as String;

      test(entry.key, skip: notCarried[type], () {
        final value = readValue(
          serializer,
          type,
          testCase["document"] as String,
        );
        final expected = testCase["expect"];

        if (expected == null) {
          expect(value, isNull);
          return;
        }

        expect(value, expected);
      });
    }
  });
}
