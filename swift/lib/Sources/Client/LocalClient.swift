public class LocalClient {
    public static func call<T>(_ request: Request, _ defValue: T? = nil) -> T? {
        let data = request.data

        // function name
        let functionName = XPLPC.shared.config.serializer.decodeFunctionName(data)

        if functionName.isEmpty {
            Log.e("[LocalClient : call] Function name is empty")
            return defValue
        }

        // mapping item
        let mappingItem = MappingData.shared.find(functionName)

        if mappingItem == nil {
            Log.e("[LocalClient : call] Mapping not found for function: \(functionName)")
            return defValue
        }

        // execute
        if let message = XPLPC.shared.config.serializer.decodeMessage(data) {
            if let value = mappingItem?.target(message).value?.value {
                return value as? T
            } else {
                Log.e("[LocalClient : call] Error when try to decode return value for function: \(functionName)")
                return defValue
            }
        } else {
            Log.e("[LocalClient : call] Error when try to decode message data for function: \(functionName)")
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
