import Foundation

/// Decodes a value read off the wire into a decodable type, reading every number, boolean and text inside it through the checks a value at the top is read through.
struct ValueDecoder: Decoder {
    let value: Any
    let codingPath: [CodingKey]

    var userInfo: [CodingUserInfoKey: Any] {
        [:]
    }

    init(_ value: Any, codingPath: [CodingKey] = []) {
        self.value = value
        self.codingPath = codingPath
    }

    func read<D: Decodable>(_ type: D.Type) throws -> D {
        // A date travels as whole milliseconds since the unix epoch, which is not the shape its own decoding reads.
        if D.self == Date.self {
            guard let milliseconds: Int64 = ValueReader.exact(value),
                  let date = Date(timeIntervalSince1970: Double(milliseconds) / 1000) as? D
            else {
                throw mismatch(type)
            }

            return date
        }

        if D.self is any BinaryInteger.Type || D.self is any BinaryFloatingPoint.Type || D.self == Bool.self || D.self == String.self {
            guard let scalar: D = ValueReader.exact(value) else {
                throw mismatch(type)
            }

            return scalar
        }

        // A decimal, a url and data are written by the encoder in a shape of their own rather than through their own encoding, so they are read back the way its decoder reads them.
        if D.self == Decimal.self || D.self == URL.self || D.self == Data.self {
            let written = try JSONSerialization.data(withJSONObject: value, options: .fragmentsAllowed)

            return try JSONDecoder().decode(type, from: written)
        }

        return try D(from: self)
    }

    func container<Key: CodingKey>(keyedBy _: Key.Type) throws -> KeyedDecodingContainer<Key> {
        guard let object = value as? [String: Any] else {
            throw mismatch([String: Any].self)
        }

        return KeyedDecodingContainer(KeyedValues<Key>(object: object, codingPath: codingPath))
    }

    func unkeyedContainer() throws -> UnkeyedDecodingContainer {
        guard let array = value as? [Any] else {
            throw mismatch([Any].self)
        }

        return UnkeyedValues(array: array, codingPath: codingPath)
    }

    func singleValueContainer() throws -> SingleValueDecodingContainer {
        SingleValue(decoder: self)
    }

    private func mismatch(_ type: Any.Type) -> DecodingError {
        DecodingError.typeMismatch(type, .init(codingPath: codingPath, debugDescription: "The value is not a \(type)"))
    }
}

private struct SuperKey: CodingKey {
    let stringValue = "super"
    let intValue: Int? = nil

    init() {}

    init?(stringValue _: String) {
        nil
    }

    init?(intValue _: Int) {
        nil
    }
}

private struct IndexKey: CodingKey {
    let intValue: Int?
    let stringValue: String

    init(_ index: Int) {
        intValue = index
        stringValue = String(index)
    }

    init?(intValue: Int) {
        self.init(intValue)
    }

    init?(stringValue _: String) {
        nil
    }
}

private struct SingleValue: SingleValueDecodingContainer {
    let decoder: ValueDecoder

    var codingPath: [CodingKey] {
        decoder.codingPath
    }

    func decodeNil() -> Bool {
        decoder.value is NSNull
    }

    func decode(_ type: Bool.Type) throws -> Bool {
        try decoder.read(type)
    }

    func decode(_ type: String.Type) throws -> String {
        try decoder.read(type)
    }

    func decode(_ type: Double.Type) throws -> Double {
        try decoder.read(type)
    }

    func decode(_ type: Float.Type) throws -> Float {
        try decoder.read(type)
    }

    func decode(_ type: Int.Type) throws -> Int {
        try decoder.read(type)
    }

    func decode(_ type: Int8.Type) throws -> Int8 {
        try decoder.read(type)
    }

    func decode(_ type: Int16.Type) throws -> Int16 {
        try decoder.read(type)
    }

    func decode(_ type: Int32.Type) throws -> Int32 {
        try decoder.read(type)
    }

    func decode(_ type: Int64.Type) throws -> Int64 {
        try decoder.read(type)
    }

    func decode(_ type: UInt.Type) throws -> UInt {
        try decoder.read(type)
    }

    func decode(_ type: UInt8.Type) throws -> UInt8 {
        try decoder.read(type)
    }

    func decode(_ type: UInt16.Type) throws -> UInt16 {
        try decoder.read(type)
    }

    func decode(_ type: UInt32.Type) throws -> UInt32 {
        try decoder.read(type)
    }

    func decode(_ type: UInt64.Type) throws -> UInt64 {
        try decoder.read(type)
    }

    func decode<T: Decodable>(_ type: T.Type) throws -> T {
        try decoder.read(type)
    }
}

private struct KeyedValues<Key: CodingKey>: KeyedDecodingContainerProtocol {
    let object: [String: Any]
    let codingPath: [CodingKey]

    var allKeys: [Key] {
        object.keys.compactMap { Key(stringValue: $0) }
    }

    func contains(_ key: Key) -> Bool {
        object[key.stringValue] != nil
    }

    func decodeNil(forKey key: Key) throws -> Bool {
        try decoder(for: key).value is NSNull
    }

    func decode(_ type: Bool.Type, forKey key: Key) throws -> Bool {
        try decoder(for: key).read(type)
    }

    func decode(_ type: String.Type, forKey key: Key) throws -> String {
        try decoder(for: key).read(type)
    }

    func decode(_ type: Double.Type, forKey key: Key) throws -> Double {
        try decoder(for: key).read(type)
    }

    func decode(_ type: Float.Type, forKey key: Key) throws -> Float {
        try decoder(for: key).read(type)
    }

    func decode(_ type: Int.Type, forKey key: Key) throws -> Int {
        try decoder(for: key).read(type)
    }

    func decode(_ type: Int8.Type, forKey key: Key) throws -> Int8 {
        try decoder(for: key).read(type)
    }

    func decode(_ type: Int16.Type, forKey key: Key) throws -> Int16 {
        try decoder(for: key).read(type)
    }

    func decode(_ type: Int32.Type, forKey key: Key) throws -> Int32 {
        try decoder(for: key).read(type)
    }

    func decode(_ type: Int64.Type, forKey key: Key) throws -> Int64 {
        try decoder(for: key).read(type)
    }

    func decode(_ type: UInt.Type, forKey key: Key) throws -> UInt {
        try decoder(for: key).read(type)
    }

    func decode(_ type: UInt8.Type, forKey key: Key) throws -> UInt8 {
        try decoder(for: key).read(type)
    }

    func decode(_ type: UInt16.Type, forKey key: Key) throws -> UInt16 {
        try decoder(for: key).read(type)
    }

    func decode(_ type: UInt32.Type, forKey key: Key) throws -> UInt32 {
        try decoder(for: key).read(type)
    }

    func decode(_ type: UInt64.Type, forKey key: Key) throws -> UInt64 {
        try decoder(for: key).read(type)
    }

    func decode<T: Decodable>(_ type: T.Type, forKey key: Key) throws -> T {
        try decoder(for: key).read(type)
    }

    func nestedContainer<NestedKey: CodingKey>(keyedBy type: NestedKey.Type, forKey key: Key) throws -> KeyedDecodingContainer<NestedKey> {
        try decoder(for: key).container(keyedBy: type)
    }

    func nestedUnkeyedContainer(forKey key: Key) throws -> UnkeyedDecodingContainer {
        try decoder(for: key).unkeyedContainer()
    }

    /// A super decoder reads what the encoder wrote under its key, and a key it never wrote reads as null, the way the decoder of the encoder reads it.
    func superDecoder() throws -> Decoder {
        let key = SuperKey()

        return ValueDecoder(object[key.stringValue] ?? NSNull(), codingPath: codingPath + [key])
    }

    func superDecoder(forKey key: Key) throws -> Decoder {
        ValueDecoder(object[key.stringValue] ?? NSNull(), codingPath: codingPath + [key])
    }

    private func decoder(for key: Key) throws -> ValueDecoder {
        guard let value = object[key.stringValue] else {
            throw DecodingError.keyNotFound(key, .init(codingPath: codingPath, debugDescription: "The object carries no \(key.stringValue)"))
        }

        return ValueDecoder(value, codingPath: codingPath + [key])
    }
}

private struct UnkeyedValues: UnkeyedDecodingContainer {
    let array: [Any]
    let codingPath: [CodingKey]
    private(set) var currentIndex = 0

    var count: Int? {
        array.count
    }

    var isAtEnd: Bool {
        currentIndex >= array.count
    }

    mutating func decodeNil() throws -> Bool {
        guard try current().value is NSNull else {
            return false
        }

        currentIndex += 1

        return true
    }

    mutating func decode(_ type: Bool.Type) throws -> Bool {
        try next(type)
    }

    mutating func decode(_ type: String.Type) throws -> String {
        try next(type)
    }

    mutating func decode(_ type: Double.Type) throws -> Double {
        try next(type)
    }

    mutating func decode(_ type: Float.Type) throws -> Float {
        try next(type)
    }

    mutating func decode(_ type: Int.Type) throws -> Int {
        try next(type)
    }

    mutating func decode(_ type: Int8.Type) throws -> Int8 {
        try next(type)
    }

    mutating func decode(_ type: Int16.Type) throws -> Int16 {
        try next(type)
    }

    mutating func decode(_ type: Int32.Type) throws -> Int32 {
        try next(type)
    }

    mutating func decode(_ type: Int64.Type) throws -> Int64 {
        try next(type)
    }

    mutating func decode(_ type: UInt.Type) throws -> UInt {
        try next(type)
    }

    mutating func decode(_ type: UInt8.Type) throws -> UInt8 {
        try next(type)
    }

    mutating func decode(_ type: UInt16.Type) throws -> UInt16 {
        try next(type)
    }

    mutating func decode(_ type: UInt32.Type) throws -> UInt32 {
        try next(type)
    }

    mutating func decode(_ type: UInt64.Type) throws -> UInt64 {
        try next(type)
    }

    mutating func decode<T: Decodable>(_ type: T.Type) throws -> T {
        try next(type)
    }

    mutating func nestedContainer<NestedKey: CodingKey>(keyedBy type: NestedKey.Type) throws -> KeyedDecodingContainer<NestedKey> {
        let container = try current().container(keyedBy: type)
        currentIndex += 1

        return container
    }

    mutating func nestedUnkeyedContainer() throws -> UnkeyedDecodingContainer {
        let container = try current().unkeyedContainer()
        currentIndex += 1

        return container
    }

    mutating func superDecoder() throws -> Decoder {
        let decoder = try current()
        currentIndex += 1

        return decoder
    }

    private mutating func next<T: Decodable>(_ type: T.Type) throws -> T {
        let decoded = try current().read(type)
        currentIndex += 1

        return decoded
    }

    private func current() throws -> ValueDecoder {
        guard !isAtEnd else {
            throw DecodingError.valueNotFound(Any.self, .init(codingPath: codingPath, debugDescription: "The list holds no more values"))
        }

        return ValueDecoder(array[currentIndex], codingPath: codingPath + [IndexKey(currentIndex)])
    }
}
