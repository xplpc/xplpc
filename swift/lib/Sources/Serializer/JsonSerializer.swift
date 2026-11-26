import Foundation

public final class JsonSerializer: Serializer, Sendable {
    private let encoder = JsonCoding.encoder()

    public init() {}

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

    public func decodeFunctionReturnValue<T>(_ data: String) -> T? {
        // An empty response is the empty value every failing path answers with, not a document that failed to parse.
        if data.isEmpty {
            return nil
        }

        guard let document = parse(data, "decodeFunctionReturnValue", "Error when parse json") as? [String: Any] else {
            return nil
        }

        // A mapping with nothing to answer travels as null, so there is nothing to read.
        guard let answer = document["r"], !(answer is NSNull) else {
            return nil
        }

        guard let value: T = ValueReader.read(AnyCodable(answer)) else {
            Log.e("[JsonSerializer : decodeFunctionReturnValue] Value has another type")
            return nil
        }

        return value
    }

    public func decodeRequest(_ data: String) -> DecodedRequest? {
        guard let document = parse(data, "decodeRequest", "Error when decode request") else {
            return nil
        }

        guard let request = document as? [String: Any] else {
            Log.e("[JsonSerializer : decodeRequest] Error when decode request")
            return nil
        }

        // A request that names no function as text routes nowhere, which is what the reference refuses before it ever reaches a decoder.
        guard let functionName = request["f"] as? String, !functionName.isEmpty else {
            Log.e("[JsonSerializer : decodeRequest] The request names no function")
            return nil
        }

        let params = request["p"] ?? NSNull()

        guard params is NSNull || params is [Any] else {
            Log.e("[JsonSerializer : decodeRequest] Parameters of function \"\(functionName)\" are not a list")
            return nil
        }

        let message = Message()

        for param in params as? [Any] ?? [] {
            guard let entry = param as? [String: Any], let name = entry["n"] as? String else {
                Log.e("[JsonSerializer : decodeRequest] Function \"\(functionName)\" carries a parameter whose name is not text")
                return nil
            }

            // A name read twice would be answered by whichever copy a bridge happens to keep, so the request is refused rather than guessed at.
            guard !message.data.keys.contains(name) else {
                Log.e("[JsonSerializer : decodeRequest] Parameter \"\(name)\" of function \"\(functionName)\" appears more than once")
                return nil
            }

            let value = entry["v"] ?? NSNull()

            // A bare nil would be read as the nil literal AnyCodable accepts, which is a value rather than its absence.
            message.set(name, value is NSNull ? Optional.none : AnyCodable(value))
        }

        return DecodedRequest(functionName: functionName, message: message)
    }

    /// The document is read keeping whether each number was written as an integer or a fraction, which decoding into swift types forgets.
    private func parse(_ data: String, _ operation: String, _ failure: String) -> Any? {
        do {
            return try JSONSerialization.jsonObject(with: Data(data.utf8), options: .fragmentsAllowed)
        } catch let e {
            Log.e("[JsonSerializer : \(operation)] \(failure)")
            Log.d("[JsonSerializer : \(operation)] \(failure): \(e.localizedDescription)")
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

    struct JsonFunctionReturnValueData<T: Encodable>: Encodable {
        var r: T?

        init(_ r: T?) {
            self.r = r
        }
    }
}
