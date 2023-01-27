public class Request {
    private(set) var data: String

    public init(_ functionName: String, _ params: Param...) {
        data = XPLPC.shared.config.serializer.encodeRequest(functionName, params)
    }
}
