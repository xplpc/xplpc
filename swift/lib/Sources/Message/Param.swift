public class Param: Codable {
    public var n: String
    public var v: AnyCodable?

    private enum CodingKeys: String, CodingKey {
        case n
        case v
    }

    public init(_ n: String, _ v: AnyCodable?) {
        self.n = n
        self.v = v
    }

    public init(_ n: String, _ v: Any?) {
        self.n = n

        if let v = v as? AnyCodable {
            self.v = v
        } else {
            self.v = AnyCodable(v)
        }
    }

    public func encode(to encoder: Encoder) throws {
        // A parameter without a value keeps its name and travels as null, which the synthesized encoding would leave out.

        var container = encoder.container(keyedBy: CodingKeys.self)

        try container.encode(n, forKey: .n)

        if let v {
            try container.encode(v, forKey: .v)
        } else {
            try container.encodeNil(forKey: .v)
        }
    }
}
