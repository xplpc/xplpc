public class DecodedRequest {
    public let functionName: String
    public let message: Message

    public init(functionName: String, message: Message) {
        self.functionName = functionName
        self.message = message
    }
}
