public protocol Serializer {
    func encodeRequest(_ functionName: String, _ params: [Param]) -> String
    func decodeFunctionName(_ data: String) -> String
    func decodeFunctionReturnValue<T: Decodable>(_ data: String) -> T?
    func decodeMessage(_ data: String) -> Message?
    func encodeFunctionReturnValue(_ data: ResponseData?) -> String
}
