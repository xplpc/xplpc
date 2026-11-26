public final class Config: Sendable {
    let serializer: Serializer

    public init(serializer: Serializer) {
        self.serializer = serializer
    }
}
