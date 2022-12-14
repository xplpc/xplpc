public class RemoteClient {
    internal class RemoteClientReturn<T: Decodable>: Decodable {
        var r: T?
    }

    public static func call<T: Decodable>(_ request: Request, _ defValue: T? = nil) -> T? {
        let json = PlatformProxy.shared.call(request.data) ?? ""

        if let response: RemoteClientReturn<T> = XPLPC.shared.config.serializer.decodeFunctionReturnValue(json) {
            return response.r
        } else {
            Log.e("[RemoteClient : call] Error when try to decode return value")
            return defValue
        }
    }

    public static func callAsync<T: Decodable>(_ request: Request, _ defValue: T? = nil, _ completion: ((_ result: T?) -> Void)? = nil) {
        DispatchQueue.global().async {
            let result = call(request, defValue)
            completion?(result)
        }
    }

    #if compiler(>=5.7) && canImport(_Concurrency)
        @available(macOS 12, iOS 13, tvOS 13, watchOS 6, *)
        public static func callAsync<T: Decodable>(_ request: Request, _ defValue: T? = nil) async -> T? {
            return call(request, defValue)
        }
    #endif
}
