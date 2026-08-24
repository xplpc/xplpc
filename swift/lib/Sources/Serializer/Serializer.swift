public protocol Serializer {
    func encodeRequest(_ functionName: String, _ params: [Param]) -> String
    func decodeFunctionReturnValue<T: Decodable>(_ data: String) -> T?
    func decodeRequest(_ data: String) -> DecodedRequest?
    func encodeFunctionReturnValue(_ data: ResponseData?) -> String
}
