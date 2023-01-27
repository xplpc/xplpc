public class Param: Codable {
    public var n: String
    public var v: AnyCodable?

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
}
