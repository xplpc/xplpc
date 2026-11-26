import Foundation
import XCTest
import xplpc

/// Runs every case of the shared conformance suite, so a case added to the repository is read here without code, and a failure names the file it came from.
final class ConformanceTest: XPLPCTestCase {
    func testWireEncodesTheSameDocument() {
        // The request is encoded by the serializer `Request` uses, since its initializer is variadic and cannot take a list read at runtime.

        forEachCase("wire") { label, fields in
            guard let function = fields["function"] as? String,
                  let declared = fields["params"] as? [[String: Any]],
                  let bytes = fields["bytes"] as? String
            else {
                XCTFail("\(label) does not carry a function, its parameters and the bytes")
                return
            }

            let params = declared.compactMap { param(of: $0, in: label) }

            guard params.count == declared.count else {
                XCTFail("\(label) declares a parameter this runner cannot build")
                return
            }

            let data = JsonSerializer().encodeRequest(function, params)

            // JSONEncoder does not keep key order, so the bytes are compared as a document, then for their length and for whitespace outside a string.
            XCTAssertEqual(canonical(bytes), canonical(data), "\(label) encodes another document: \(data)")
            XCTAssertEqual(bytes.utf8.count, data.utf8.count, "\(label) encodes another number of bytes: \(data)")
            XCTAssertFalse(hasWhitespaceOutsideStrings(data), "\(label) encodes whitespace outside a string: \(data)")
        }
    }

    func testAcceptReadsTheFunctionAndItsParameters() {
        forEachCase("accept") { label, fields in
            guard let document = fields["document"] as? String,
                  let function = fields["function"] as? String,
                  let params = fields["params"] as? [String: Any]
            else {
                XCTFail("\(label) does not carry a document, a function and its parameters")
                return
            }

            guard let request = JsonSerializer().decodeRequest(document) else {
                XCTFail("\(label) was refused by the decoder while every bridge reads it")
                return
            }

            XCTAssertEqual(function, request.functionName, "\(label) names another function")

            for (name, expected) in params {
                assertParam(name, expected, request.message, label)
            }
        }
    }

    func testRefuseAnswersNothing() {
        forEachCase("refuse") { label, fields in
            guard let document = fields["document"] as? String else {
                XCTFail("\(label) does not carry a document")
                return
            }

            let request = JsonSerializer().decodeRequest(document)

            XCTAssertNil(request, "\(label) was read by the decoder as function \"\(request?.functionName ?? "")\" while every bridge refuses it")
        }
    }

    func testValuesReadTheDeclaredType() {
        forEachCase("values") { label, fields in
            guard let document = fields["document"] as? String, let type = fields["type"] as? String, let expected = fields["expect"] else {
                XCTFail("\(label) does not carry a document, a type and an expectation")
                return
            }

            guard let reader = readers[type] else {
                XCTFail("\(label) declares a type this runner does not know: \(type)")
                return
            }

            reader(document, expected, label)
        }
    }

    private var readers: [String: (String, Any, String) -> Void] {
        [
            "bool": { self.assertRead(Bool.self, $0, $1, $2) },
            "int8": { self.assertRead(Int8.self, $0, $1, $2) },
            "int16": { self.assertRead(Int16.self, $0, $1, $2) },
            "int32": { self.assertRead(Int32.self, $0, $1, $2) },
            "int64": { self.assertRead(Int64.self, $0, $1, $2) },
            "uint8": { self.assertRead(UInt8.self, $0, $1, $2) },
            "uint16": { self.assertRead(UInt16.self, $0, $1, $2) },
            "uint32": { self.assertRead(UInt32.self, $0, $1, $2) },
            "uint64": { self.assertRead(UInt64.self, $0, $1, $2) },
            "float": { self.assertRead(Float.self, $0, $1, $2) },
            "double": { self.assertRead(Double.self, $0, $1, $2) },
            "string": { self.assertRead(String.self, $0, $1, $2) },
            "char": { self.assertRead(CodePoint.self, $0, $1, $2) },
            "date": { self.assertRead(Date.self, $0, $1, $2) },
            "dataview": { self.assertRead(DataView.self, $0, $1, $2) },
            "list<int32>": { self.assertRead([Int32].self, $0, $1, $2) },
            "list<string>": { self.assertRead([String].self, $0, $1, $2) },
            "list<list<int32>>": { self.assertRead([[Int32]].self, $0, $1, $2) },
            "map<int32>": { self.assertRead([String: Int32].self, $0, $1, $2) },
            "todo": { self.assertRead(Todo.self, $0, $1, $2) },
        ]
    }

    /// Compares a value read with what the case expects in the form the wire writes it, except a float, which writing would widen, and a view, which the case names by its handle.
    private func assertRead<T: Encodable>(_: T.Type, _ document: String, _ expected: Any, _ label: String) {
        let value: T? = JsonSerializer().decodeFunctionReturnValue(document)

        if expected is NSNull {
            XCTAssertNil(value, "\(label) reads a value every bridge refuses")
            return
        }

        guard let value else {
            XCTFail("\(label) refuses a value every bridge reads")
            return
        }

        if let float = value as? Float {
            XCTAssertEqual((expected as? NSNumber)?.floatValue, float, "\(label) reads another value")
            return
        }

        if let view = value as? DataView {
            XCTAssertEqual((expected as? NSNumber)?.uint64Value, view.buffer, "\(label) reads another handle")
            return
        }

        XCTAssertEqual(canonical(value: expected), canonical(value: written(value)), "\(label) reads another value")
    }

    private func written(_ value: some Encodable) -> Any? {
        let answer = JsonSerializer().encodeFunctionReturnValue(AnyCodable(value))

        guard let object = try? JSONSerialization.jsonObject(with: Data(answer.utf8)) as? [String: Any] else {
            return nil
        }

        return object["r"]
    }

    private func canonical(value: Any?) -> String? {
        guard let value, let data = try? JSONSerialization.data(withJSONObject: [value], options: [.sortedKeys]) else {
            return nil
        }

        return String(data: data, encoding: .utf8)
    }

    private func forEachCase(_ group: String, _ body: (String, [String: Any]) -> Void) {
        XCTAssertTrue(ConformanceFiles.found, "The conformance cases could not be located")

        let names = ConformanceFiles.names(group)

        XCTAssertFalse(names.isEmpty, "The \(group) group carries no case")

        for name in names {
            let label = "\(group)/\(name).json"

            guard let data = ConformanceFiles.read(group, name),
                  let fields = (try? JSONSerialization.jsonObject(with: data)) as? [String: Any]
            else {
                XCTFail("\(label) is not a json object")
                continue
            }

            body(label, fields)
        }
    }

    private func param(of declared: [String: Any], in label: String) -> Param? {
        guard let name = declared["name"] as? String, let type = declared["type"] as? String else {
            XCTFail("\(label) declares a parameter without a name or a type")
            return nil
        }

        guard let write = writers[type] else {
            XCTFail("\(label) declares a parameter type this runner does not know: \(type)")
            return nil
        }

        return write(declared["value"]).map { Param(name, $0) }
    }

    /// Builds each parameter as the type a swift caller sends, where an absent value is the one a parameter without a value carries.
    private var writers: [String: (Any?) -> AnyCodable??] {
        [
            "int": { ($0 as? NSNumber).map { AnyCodable($0.int64Value) } },
            "int64": { ($0 as? NSNumber).map { AnyCodable($0.int64Value) } },
            "uint64": { ($0 as? NSNumber).map { AnyCodable($0.uint64Value) } },
            "double": { ($0 as? NSNumber).map { AnyCodable($0.doubleValue) } },
            "string": { ($0 as? String).map { AnyCodable($0) } },
            "bool": { ($0 as? NSNumber).map { AnyCodable($0.boolValue) } },
            "null": { _ in .some(Optional.none) },
            "char": { ($0 as? NSNumber).flatMap { Unicode.Scalar($0.uint32Value) }.map { AnyCodable(CodePoint($0)) } },
            "date": { ($0 as? NSNumber).map { AnyCodable(Date(timeIntervalSince1970: $0.doubleValue / 1000)) } },
            "dataview": { ($0 as? NSNumber).map { AnyCodable(DataView(buffer: $0.uint64Value)) } },
            "list<int>": { ($0 as? [NSNumber]).map { AnyCodable($0.map(\.int64Value)) } },
            "list<string>": { ($0 as? [String]).map { AnyCodable($0) } },
            "list<list<int>>": { ($0 as? [[NSNumber]]).map { AnyCodable($0.map { $0.map(\.int64Value) }) } },
            "list<dataview>": { ($0 as? [NSNumber]).map { AnyCodable($0.map { DataView(buffer: $0.uint64Value) }) } },
            "map<int>": { ($0 as? [String: NSNumber]).map { AnyCodable($0.mapValues(\.int64Value)) } },
            "record": { value in
                guard let fields = value as? [String: Any], let count = fields["count"] as? NSNumber, let name = fields["name"] as? String else {
                    return nil
                }

                return AnyCodable(Recorded(count: count.int64Value, name: name))
            },
        ]
    }

    private func assertParam(_ name: String, _ expected: Any, _ message: Message, _ label: String) {
        if expected is NSNull {
            let value: Any? = message.get(name)
            XCTAssertNil(value, "\(label) reads parameter \"\(name)\" as a value")
            return
        }

        if let text = expected as? String {
            XCTAssertEqual(text, message.get(name) as String?, "\(label) reads parameter \"\(name)\"")
            return
        }

        guard let number = expected as? NSNumber else {
            XCTFail("\(label) expects parameter \"\(name)\" to be a value this runner does not know")
            return
        }

        if CFGetTypeID(number) == CFBooleanGetTypeID() {
            XCTAssertEqual(number.boolValue, message.get(name) as Bool?, "\(label) reads parameter \"\(name)\"")
            return
        }

        XCTAssertEqual(number.int64Value, message.get(name) as Int64?, "\(label) reads parameter \"\(name)\"")
    }

    /// Writes a document again with sorted keys, so two encodings of the same document compare equal whatever order their keys were in.
    private func canonical(_ text: String) -> String? {
        guard let object = try? JSONSerialization.jsonObject(with: Data(text.utf8)),
              let data = try? JSONSerialization.data(withJSONObject: object, options: [.sortedKeys])
        else {
            return nil
        }

        return String(data: data, encoding: .utf8)
    }

    private func hasWhitespaceOutsideStrings(_ text: String) -> Bool {
        var inString = false
        var escaped = false

        for character in text {
            if escaped {
                escaped = false
                continue
            }

            if inString, character == "\\" {
                escaped = true
                continue
            }

            if character == "\"" {
                inString.toggle()
                continue
            }

            if !inString, character.isWhitespace {
                return true
            }
        }

        return false
    }
}

/// A record of the caller's own whose fields are written in the order every encoder agrees on.
private struct Recorded: Encodable {
    let count: Int64
    let name: String
}
