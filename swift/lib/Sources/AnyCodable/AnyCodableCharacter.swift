extension Character: Codable {
    public init(from decoder: Decoder) throws {
        let container = try decoder.singleValueContainer()
        let value = try container.decode(Int.self)

        if let unicodeScalar = UnicodeScalar(value) {
            self = Character(unicodeScalar)
        } else {
            self = "\u{FFFF}"
        }
    }

    public func encode(to encoder: Encoder) throws {
        var container = encoder.singleValueContainer()
        try container.encode(unicodeScalars.first!.value)
    }
}
