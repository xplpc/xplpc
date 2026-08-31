import Foundation

public class JsonSerializer: Serializer {
    private let decoder = JSONDecoder()
    private let encoder = JSONEncoder()

    public init() {
        // A date travels as whole milliseconds since the unix epoch, which no built in strategy produces.
        encoder.dateEncodingStrategy = .custom { date, encoder in
            var container = encoder.singleValueContainer()
            try container.encode(Int64((date.timeIntervalSince1970 * 1000).rounded()))
        }

        decoder.dateDecodingStrategy = .custom { decoder in
            let container = try decoder.singleValueContainer()
            let milliseconds = try container.decode(Int64.self)

            return Date(timeIntervalSince1970: Double(milliseconds) / 1000)
        }
    }

    public func encodeRequest(_ functionName: String, _ params: [Param]) -> String {
        do {
            let request = JsonRequestData(functionName, params)
            let data = try encoder.encode(request)

            guard let result = String(data: data, encoding: .utf8) else {
                Log.e("[JsonSerializer : encodeRequest] Error when encode data")

                return ""
            }

            return result
        } catch let e {
            Log.e("[JsonSerializer : encodeRequest] Error when encode data")
            Log.d("[JsonSerializer : encodeRequest] Error when encode data: \(e.localizedDescription)")
        }

        return ""
    }

    public func decodeFunctionReturnValue<T: Decodable>(_ data: String) -> T? {
        // An empty response is the empty value every failing path answers with, not a document that failed to parse.
        if data.isEmpty {
            return nil
        }

        do {
            let jsonData = Data(data.utf8)
            return try decoder.decode(T.self, from: jsonData)
        } catch let e {
            Log.e("[JsonSerializer : decodeFunctionReturnValue] Error when parse json")
            Log.d("[JsonSerializer : decodeFunctionReturnValue] Error when parse json: \(e.localizedDescription)")
        }

        return nil
    }

    public func decodeRequest(_ data: String) -> DecodedRequest? {
        do {
            let jsonData = Data(data.utf8)
            let decoded = try decoder.decode(JsonRequestReadData.self, from: jsonData)
            let message = Message()

            if let params = decoded.p {
                for p in params {
                    message.set(p.n, p.v)
                }
            }

            return DecodedRequest(functionName: decoded.f ?? "", message: message)
        } catch let e {
            Log.e("[JsonSerializer : decodeRequest] Error when decode request")
            Log.d("[JsonSerializer : decodeRequest] Error when decode request: \(e.localizedDescription)")
        }

        return nil
    }

    public func encodeFunctionReturnValue(_ data: ResponseData?) -> String {
        do {
            let value = JsonFunctionReturnValueData<ResponseData?>(data)
            let jsonData = try encoder.encode(value)

            guard let result = String(data: jsonData, encoding: .utf8) else {
                Log.e("[JsonSerializer : encodeFunctionReturnValue] Error when encode data")

                return ""
            }

            return result
        } catch let e {
            Log.e("[JsonSerializer : encodeFunctionReturnValue] Error when encode data")
            Log.d("[JsonSerializer : encodeFunctionReturnValue] Error when encode data: \(e.localizedDescription)")
        }

        return ""
    }

    struct JsonRequestData: Encodable {
        let f: String
        var p: [Param]?

        init(_ f: String, _ p: [Param]?) {
            self.f = f
            self.p = p
        }
    }

    struct JsonRequestReadData: Decodable {
        var f: String?
        var p: [Param]?
    }

    struct JsonFunctionReturnValueData<T: Encodable>: Encodable {
        var r: T?

        init(_ r: T?) {
            self.r = r
        }
    }
}
