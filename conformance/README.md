# Conformance cases

Every bridge reads and writes the wire format with a serializer of its own, so these cases are the one place the format is written down as data, and every suite with a serializer of its own runs every case of the groups below that apply to it. A case is a json file, and adding one needs no code in any runner.

| Group | A case holds | What a runner asserts |
| --- | --- | --- |
| `wire` | a function, typed parameters and the exact bytes | the request it builds encodes to those bytes, and on swift, whose encoder does not keep key order, the same document with no whitespace |
| `accept` | a document, the function it names and the parameters it carries | the decoder reads that function and those values |
| `refuse` | a document | the decoder refuses it and answers nothing |
| `values` | an answer, a declared type and the value expected, where null means refused | the typed reader answers that value |

The `values` group runs on c++, kotlin, swift, dart and python, which are the bridges that read an answer as a declared type. Typescript erases the type, so it has none to check against. The [types guide](../docs/types.md) says what every case below means for each language.

A case names its type with one of these, and a runner that meets any other fails the case rather than skipping it:

| Group | Types |
| --- | --- |
| `wire` | `null`, `bool`, `int`, `int64`, `uint64`, `double`, `string`, `char`, `date`, `dataview`, `list<int>`, `list<string>`, `list<list<int>>`, `list<dataview>`, `map<int>`, `record` |
| `values` | `bool`, `int8`, `int16`, `int32`, `int64`, `uint8`, `uint16`, `uint32`, `uint64`, `float`, `double`, `string`, `char`, `date`, `dataview`, `list<int32>`, `list<string>`, `list<list<int32>>`, `map<int32>`, `todo` |

An `int` holds an integer every language writes exactly, which is one within two to the fifty three, while `int64` and `uint64` hold the full range. A `record` is an object of the caller's own with the fields `count` and `name`, and a `todo` is the sample type with `id`, `title`, `body`, `data` and `done`. A `values` case expects a character as its code point, a date as its milliseconds and a data view as its handle.

A language that has no equivalent of a type skips its cases by name and says why in its runner, which the types guide lists.

A document that writes a key twice in any object, its function name included, is refused where requests are routed, which is the core, and no host decoder ever receives one, so those cases are pinned by the c++ suite rather than here.
