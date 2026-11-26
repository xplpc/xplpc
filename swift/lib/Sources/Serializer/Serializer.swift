public protocol Serializer: Sendable {
    func encodeRequest(_ functionName: String, _ params: [Param]) -> String
    func decodeFunctionReturnValue<T>(_ data: String) -> T?
    func decodeRequest(_ data: String) -> DecodedRequest?
    func encodeFunctionReturnValue(_ data: ResponseData?) -> String
}
