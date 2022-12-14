public class Response: Codable {
    private(set) var value: AnyCodable?

    public init(_ value: AnyCodable?) {
        self.value = value
    }
}
