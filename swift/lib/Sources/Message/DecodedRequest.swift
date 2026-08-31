public class DecodedRequest {
    public let functionName: String
    public let message: Message

    init(functionName: String, message: Message) {
        self.functionName = functionName
        self.message = message
    }
}
