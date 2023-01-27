public class Config {
    private(set) var serializer: Serializer

    public init(serializer: Serializer) {
        self.serializer = serializer
    }
}
