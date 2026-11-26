/// A single unicode code point, the representation every platform uses for a character on the wire.
/// Swift's `Character` is an extended grapheme cluster and may hold several scalars, so it cannot round trip through the integer the other platforms send.
public struct CodePoint: Codable, Hashable, Sendable {
    public let scalar: Unicode.Scalar

    public var character: Character {
        Character(scalar)
    }

    public init(_ scalar: Unicode.Scalar) {
        self.scalar = scalar
    }

    public init?(_ character: Character) {
        guard character.unicodeScalars.count == 1, let scalar = character.unicodeScalars.first else {
            return nil
        }

        self.scalar = scalar
    }

    public init(from decoder: Decoder) throws {
        let container = try decoder.singleValueContainer()
        let value = try container.decode(UInt32.self)

        guard let scalar = Unicode.Scalar(value) else {
            throw DecodingError.dataCorruptedError(
                in: container,
                debugDescription: "\(value) is not a unicode scalar value"
            )
        }

        self.scalar = scalar
    }

    public func encode(to encoder: Encoder) throws {
        var container = encoder.singleValueContainer()
        try container.encode(scalar.value)
    }
}

extension CodePoint: ExpressibleByUnicodeScalarLiteral {
    public init(unicodeScalarLiteral value: Unicode.Scalar) {
        self.init(value)
    }
}
