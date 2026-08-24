public class Request {
    public private(set) var data: String

    public init(_ functionName: String, _ params: Param...) {
        guard let config = XPLPC.shared.config else {
            Log.e("[Request : init] XPLPC was not initialized")
            data = ""
            return
        }

        data = config.serializer.encodeRequest(functionName, params)
    }
}
