public class JsonSerializer: Serializer {
    private var decoder = JSONDecoder()
    private var encoder = JSONEncoder()

    public init() {
        //
    }

    public func encodeRequest(_ functionName: String, _ params: [Param]) -> String {
        do {
            let request = JsonRequestData(functionName, params)
            let data = try encoder.encode(request)

            if let result = String(data: data, encoding: .utf8) {
                return result
            } else {
                Log.e("[JsonSerializer : encodeRequest] Error when try to encode data")
            }
        } catch let e {
            Log.e("[JsonSerializer : encodeRequest] Error when try to encode data: \(e.localizedDescription)")
        }

        return ""
    }

    public func decodeFunctionName(_ data: String) -> String {
        do {
            let jsonData = Data(data.utf8)
            return try decoder.decode(JsonFunctionNameData.self, from: jsonData).f
        } catch let e {
            Log.e("[JsonSerializer : decodeFunctionName] Error when parse json: \(e.localizedDescription)")
        }

        return ""
    }

    public func decodeFunctionReturnValue<T: Decodable>(_ data: String) -> T? {
        do {
            let jsonData = Data(data.utf8)
            return try decoder.decode(T.self, from: jsonData)
        } catch let e {
            Log.e("[JsonSerializer : decodeFunctionReturnValue] Error when parse json: \(e.localizedDescription)")
        }

        return nil
    }

    public func decodeMessage(_ data: String) -> Message? {
        var decodedData: JsonParametersData?

        // decode parameters
        do {
            let jsonData = Data(data.utf8)
            decodedData = try decoder.decode(JsonParametersData.self, from: jsonData)
        } catch let e {
            Log.e("[JsonSerializer : decodeMessage] Error when get parameters data: \(e.localizedDescription)")
        }

        if let decodedData = decodedData {
            // message data
            let message = Message()

            if let params = decodedData.p {
                for p in params {
                    message.set(p.n, p.v)
                }
            }

            return message
        } else {
            Log.e("[JsonSerializer : decodeMessage] Error when try to decode data")
        }

        return nil
    }

    public func encodeFunctionReturnValue(_ data: Response?) -> String {
        do {
            let value = JsonFunctionReturnValueData<AnyCodable?>(data?.value)
            let jsonData = try encoder.encode(value)

            if let result = String(data: jsonData, encoding: .utf8) {
                return result
            } else {
                Log.e("[JsonSerializer : encodeFunctionReturnValue] Error when try to encode data")
            }
        } catch let e {
            Log.e("[JsonSerializer : encodeFunctionReturnValue] Error when try to encode data: \(e.localizedDescription)")
        }

        return ""
    }

    internal struct JsonRequestData: Encodable {
        let f: String
        var p: [Param]?

        init(_ f: String, _ p: [Param]?) {
            self.f = f
            self.p = p
        }
    }

    internal struct JsonFunctionNameData: Decodable {
        let f: String

        init(_ f: String) {
            self.f = f
        }
    }

    internal struct JsonParametersData: Decodable {
        var p: [Param]?

        init(_ p: [Param]?) {
            self.p = p
        }
    }

    internal struct JsonFunctionReturnValueData<T: Encodable>: Encodable {
        var r: T?

        init(_ r: T?) {
            self.r = r
        }
    }
}
