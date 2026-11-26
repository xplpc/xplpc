import Foundation

/// Encodes a value whose type is only known at runtime, which is how a list or an object nested inside an `AnyCodable` is encoded.
struct AnyEncodable: Encodable {
    let value: Any

    init(_ value: (some Any)?) {
        self.value = value ?? ()
    }
}

@usableFromInline
protocol _AnyEncodable {
    var value: Any { get }
    init(_ value: (some Any)?)
}

extension AnyEncodable: _AnyEncodable {}

extension _AnyEncodable {
    public func encode(to encoder: Encoder) throws {
        var container = encoder.singleValueContainer()

        // Every swift number and boolean bridges to an NSNumber, so the number keeps the width it was written with and is encoded from there.
        switch value {
        case is NSNull:
            try container.encodeNil()
        case let number as NSNumber:
            try encode(nsnumber: number, into: &container)
        case is Void:
            try container.encodeNil()
        case let character as Character:
            guard let codePoint = CodePoint(character) else {
                let context = EncodingError.Context(codingPath: container.codingPath, debugDescription: "A character built from several unicode scalars has no single code point to encode")
                throw EncodingError.invalidValue(character, context)
            }

            try container.encode(codePoint)
        case let string as String:
            try container.encode(string)
        case let date as Date:
            try container.encode(date)
        case let url as URL:
            try container.encode(url)
        case let array as [Any?]:
            try container.encode(array.map { AnyEncodable($0) })
        case let dictionary as [String: Any?]:
            try container.encode(dictionary.mapValues { AnyEncodable($0) })
        case let encodable as Encodable:
            try encodable.encode(to: encoder)
        default:
            let context = EncodingError.Context(codingPath: container.codingPath, debugDescription: "AnyEncodable value cannot be encoded")
            throw EncodingError.invalidValue(value, context)
        }
    }

    private func encode(nsnumber: NSNumber, into container: inout SingleValueEncodingContainer) throws {
        // A Swift Int8 bridges to the same objective-c char a boolean does, and only the CFBoolean singleton is the boolean.
        if CFGetTypeID(nsnumber) == CFBooleanGetTypeID() {
            try container.encode(nsnumber.boolValue)
            return
        }

        switch UInt32(nsnumber.objCType.pointee) {
        case cpp_or_c99_bool_objc_encoding:
            try container.encode(nsnumber.boolValue)
        case char_objc_encoding:
            try container.encode(nsnumber.int8Value)
        case unsigned_char_objc_encoding:
            try container.encode(nsnumber.uint8Value)
        case short_objc_encoding:
            try container.encode(nsnumber.int16Value)
        case int_objc_encoding, long_objc_encoding:
            try container.encode(nsnumber.int32Value)
        case long_long_objc_encoding:
            try container.encode(nsnumber.int64Value)
        case unsigned_short_objc_encoding:
            try container.encode(nsnumber.uint16Value)
        case unsigned_int_objc_encoding, unsigned_long_objc_encoding:
            try container.encode(nsnumber.uint32Value)
        case unsigned_long_long_objc_encoding:
            try container.encode(nsnumber.uint64Value)
        case float_objc_encoding:
            try container.encode(nsnumber.floatValue)
        case double_objc_encoding:
            try container.encode(nsnumber.doubleValue)
        default:
            let context = EncodingError.Context(codingPath: container.codingPath, debugDescription: "NSNumber cannot be encoded because its type is not handled")
            throw EncodingError.invalidValue(nsnumber, context)
        }
    }
}

extension _AnyEncodable {
    public init(nilLiteral _: ()) {
        self.init(nil as Any?)
    }

    public init(booleanLiteral value: Bool) {
        self.init(value)
    }

    public init(integerLiteral value: Int) {
        self.init(value)
    }

    public init(floatLiteral value: Double) {
        self.init(value)
    }

    public init(extendedGraphemeClusterLiteral value: String) {
        self.init(value)
    }

    public init(stringLiteral value: String) {
        self.init(value)
    }

    public init(arrayLiteral elements: Any...) {
        self.init(elements)
    }

    public init(dictionaryLiteral elements: (AnyHashable, Any)...) {
        self.init([AnyHashable: Any](elements, uniquingKeysWith: { first, _ in first }))
    }
}

// The runtime type encodings are documented at https://developer.apple.com/library/archive/documentation/Cocoa/Conceptual/ObjCRuntimeGuide/Articles/ocrtTypeEncodings.html.
private let cpp_or_c99_bool_objc_encoding = "B".unicodeScalars.first?.value
private let char_objc_encoding = "c".unicodeScalars.first?.value
private let short_objc_encoding = "s".unicodeScalars.first?.value
private let int_objc_encoding = "i".unicodeScalars.first?.value
private let long_objc_encoding = "l".unicodeScalars.first?.value
private let long_long_objc_encoding = "q".unicodeScalars.first?.value
private let unsigned_char_objc_encoding = "C".unicodeScalars.first?.value
private let unsigned_short_objc_encoding = "S".unicodeScalars.first?.value
private let unsigned_int_objc_encoding = "I".unicodeScalars.first?.value
private let unsigned_long_objc_encoding = "L".unicodeScalars.first?.value
private let unsigned_long_long_objc_encoding = "Q".unicodeScalars.first?.value
private let float_objc_encoding = "f".unicodeScalars.first?.value
private let double_objc_encoding = "d".unicodeScalars.first?.value
