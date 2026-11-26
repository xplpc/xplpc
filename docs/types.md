# Types

Every call crosses the bridge as json, and every language turns that json into its own types and back. This guide is the one place that says how each type travels, what each language reads it as and what it refuses, so a value means the same thing whichever two languages it passes between.

Every rule here is written down as data under [`conformance/`](../conformance) and run by the suite of every language it applies to. The `wire` group pins how a value is written and the `values` group pins how it is read. A rule that is not in a case is not promised.

## The rules every reader follows

A value is read only as a type that holds it exactly. A reader never rounds, wraps, truncates or guesses, and a value it cannot hold is refused rather than turned into another one. A refused value answers the caller with the empty value of the call, which is `null`, `nil`, `None`, `std::nullopt` or `undefined`, and writes one error line naming where it happened.

* A boolean is only `true` or `false`, and it is never read as a number nor a number as a boolean.
* An integer type takes an integer literal that fits its width, so `2.5`, `2.0`, `true`, `"1"` and anything past its range are refused.
* A floating type rounds a fraction to the nearest value it holds, takes an integer only when it holds that integer exactly and refuses a magnitude past its range rather than reading infinity.
* Text is only a json string, and a number is never read as text.
* A number past what a double holds and the tokens `NaN` and `Infinity` are refused by every decoder.
* `null` is no value. Reading it as any type answers the empty value without a report.
* A list, a map or a record is refused whole when any value inside it is refused, at any depth.

## Each type on the wire

| Type | On the wire | Read when | Refused |
| --- | --- | --- | --- |
| Boolean | `true`, `false` | The token is a boolean | A number, text, null |
| Signed integer, 8 to 64 bits | An integer literal | It fits the width | A fraction, an integral fraction such as `2.0`, a boolean, text, a value past the width |
| Unsigned integer, 8 to 64 bits | An integer literal | It fits the width and is not negative | The same, and any negative value |
| `float` | A number | A fraction rounds to the nearest float, an integer has to be exact, so `16777217` is refused | A magnitude past the float, a boolean, text |
| `double` | A number | A fraction rounds to the nearest double, an integer has to be exact, so `9007199254740993` is refused | A magnitude past the double, `1e400`, a boolean, text |
| Text | A json string, written as the characters themselves | It is a string | A number, a boolean |
| Character | An integer code point | It is a code point the type holds | A fraction, text such as `"A"`, a boolean, a negative value |
| Date | Integer milliseconds since the unix epoch | It is an integer the declared clock can count | A fraction, text such as an iso date, a boolean |
| Data view | `{"buffer": <handle>}` | The handle is an unsigned integer that fits | Nothing is refused, since anything that is not a handle names no buffer and reads as the empty view, handle `0` |
| List | A json array | Every element reads as the element type | A non array, any element refused, a `null` element where the element type holds none |
| Map | A json object with text keys | Every value reads as the value type | A non object, any value refused, a `null` value where the value type holds none |
| Record of your own | A json object | Every field reads as its declared type | A non object, any field refused, a `null` or missing field the type declares as never empty |

A character is the one type whose range depends on the language. A C++ `char` is signed on some abis and unsigned on others, so it holds only code points 0 through 127. A Kotlin `Char` holds 0 through 65535, and a Swift `CodePoint` holds any unicode scalar.

## Each type in each language

| Type | C++ | Kotlin | Swift | Dart | Python | TypeScript |
| --- | --- | --- | --- | --- | --- | --- |
| Boolean | `bool` | `Boolean` | `Bool` | `bool` | `bool` | `boolean` |
| Signed integer | `int8_t` to `int64_t` | `Byte`, `Short`, `Int`, `Long` | `Int8` to `Int64`, `Int` | `int`, 64 bits | `int`, any size | `number`, exact up to 2^53 |
| Unsigned integer | `uint8_t` to `uint64_t` | None | `UInt8` to `UInt64`, `UInt` | `int`, up to 2^63 - 1 | `int`, any size | `number`, exact up to 2^53 |
| `float` | `float` | `Float` | `Float` | `double` | `float` | `number` |
| `double` | `double` | `Double` | `Double` | `double` | `float` | `number` |
| Text | `std::string` | `String` | `String` | `String` | `str` | `string` |
| Character | `char` | `Char` | `CodePoint` | The code point as `int` | The code point as `int` | The code point as `number` |
| Date | `std::chrono::time_point` | `java.util.Date` | `Date` | The milliseconds as `int` | The milliseconds as `int` | The milliseconds as `number` |
| Data view | `xplpc::type::DataView` | `DataView` | `DataView` | `DataView` | `DataView` | `DataView`, whose handle is a `bigint` |
| List | `std::vector`, `std::list`, `std::deque`, `std::forward_list`, `std::set`, `std::multiset`, `std::unordered_set`, `std::unordered_multiset`, `std::array` of the exact size | `List`, `Set` | `Array`, `Set` | `List`, decoded without an element type | `list` | `Array` |
| Map | `std::map` and `std::unordered_map` keyed by `std::string` | `Map<String, T>` | `[String: T]` | `Map`, decoded without a value type | `dict` | An object |
| Record | A type with an `adl_serializer` | Any class | Any `Codable` type | A class annotated `@reflector` with a `fromJson` constructor | A dataclass or a class with `from_json` | A plain object |
| No value | `std::optional<T>` | `T?` | `Optional` | `null` | `None` | `null` or `undefined` |

C++ also reads a `std::pair` and a `std::tuple` from an array of exactly their size, a map keyed by anything but text from a list of key and value pairs, and an enumeration from the integer under it, or from the text its own serializer maps. Swift reads a `Decimal`, a `URL` and `Data` in the shape `JSONEncoder` writes them, which is a number, text and base64 text.

A language with one integer type checks a width only where a caller narrows the value itself. A Dart `int` and a Python `int` read any integer the wire carries, and a mapping that declares a narrower width checks it before using the value.

TypeScript erases its types when it compiles, so a generic read cannot be checked at runtime and answers whatever the wire carried. An integer past `Number.MAX_SAFE_INTEGER` is refused rather than rounded, and a field that needs the full 64 bit range travels as text.

## Records of your own

A record is any type of your own that crosses the bridge, and no language needs an annotation or a generator to send one.

* A field the type declares as possibly empty, such as `String?`, `Optional`, `std::optional` or `None`, reads as the empty value when the wire carries `null` or leaves the field out.
* A field the type declares as never empty refuses the whole record when the wire carries `null` for it, on C++, Kotlin and Swift, so an application never receives an object holding `null` where its type says it cannot. A Dart class decides in its own `fromJson`, and a cast such as `json['name'] as String` refuses it there.
* A field the wire leaves out takes the default the type declares, which is a Kotlin constructor default or the fallback a C++ `JsonField::read` names. A Swift `Decodable`, a Kotlin field without a default and a Python dataclass field without a default refuse a record that leaves the field out.
* A field the record does not declare is ignored.
* Python fills each field with whatever the wire carried, since an annotation there is not a type the language checks.

Kotlin fills a Kotlin class through its primary constructor with Kotlin reflection, which is what lets it see what the JVM erases. It refuses a `null` or a missing field the class declares as never null, applies the defaults the constructor declares, runs every `init` block, so a class that refuses a value there refuses it here, and checks every list, map and type argument a caller asked for, so `List<Int>` refuses `[1,null]` while `List<Int?>` reads it. A Java class carries no nullability, so Gson fills it the way it always does.

## How a value is written

Every encoder writes the same bytes for the same value, which is what the `wire` group checks on all six languages.

* The document carries no whitespace.
* Text outside ascii, markup characters and a slash are written as themselves rather than escaped.
* A character and a date are written as integers.
* A data view is written as `{"buffer": <handle>}` wherever it sits, inside a list or a record included.
* A parameter without a value and a mapping with nothing to answer are written as `null`.
* A number that is not finite has no json token. Kotlin, Swift, Dart and Python refuse it with a report, while C++ and TypeScript write `null`.
* A whole number held by a floating type is written `2.0` by C++, Kotlin, Dart and Python and `2` by Swift and TypeScript. Every floating reader reads both, and an integer reader refuses `2.0`, so a value that must arrive as an integer is sent from an integer type.
* Key order is not part of the format. C++ writes an object's keys sorted, while the other languages write them in the order the type declares them.

TypeScript refuses a `Date`, a `Map`, a `Set`, an `ArrayBuffer` and a typed array with a report, since `JSON.stringify` would write each in a shape no other language reads as what was sent.

## What each suite runs

| Group | Runs on | Skipped by name |
| --- | --- | --- |
| `wire` | C++, Kotlin, Swift, Dart, Python, TypeScript | An unsigned 64 bit integer on Kotlin, Dart and TypeScript, a signed 64 bit integer past 2^53 on TypeScript, and a character and a date on Dart, Python and TypeScript, which send the code point and the milliseconds as integers |
| `values` | C++, Kotlin, Swift, Dart, Python | Unsigned types on Kotlin, an unsigned 64 bit integer, a character, a date, a typed list, a typed map and a record on Dart, and a character, a date, a typed list, a typed map and a record on Python |

A runner that meets a type it does not know fails the case rather than skipping it, so a misspelled type is never silently passed over. Each skip names its reason in the runner, and the [conformance readme](../conformance/README.md) lists the type names a case can declare.

The edges a case carries are the smallest and largest value of each width, one past each, a fraction, an integral fraction, a boolean, text and null, and for every list, map and record a value inside it that breaks a rule. The C++ suite also checks every standard container, pair, tuple and enumeration in `SerializerTest`, and the Kotlin suite checks constructor defaults, `init` blocks and type arguments in its own `SerializerTest`.

## Adding a type

1. Write a `values` case for every edge the type carries and a `wire` case for how it is written.
2. Teach every runner the type, or list it as skipped with the reason its language cannot carry it.
3. Add its row to the tables above.
