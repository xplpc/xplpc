public class ProxyClient {
    public static func call(_ data: String) -> String {
        // function name
        let functionName = XPLPC.shared.config.serializer.decodeFunctionName(data)

        if functionName.isEmpty {
            Log.e("[ProxyClient : call] Function name is empty")
            return ""
        }

        // mapping item
        let mappingItem = MappingData.shared.find(functionName)

        if mappingItem == nil {
            Log.e("[ProxyClient : call] Mapping not found for function: \(functionName)")
            return ""
        }

        // execute
        let message = XPLPC.shared.config.serializer.decodeMessage(data)

        if let message = message {
            if let response = mappingItem?.target(message) {
                return XPLPC.shared.config.serializer.encodeFunctionReturnValue(response)
            }
        } else {
            Log.e("[ProxyClient : call] Error when try to decode message data for function: \(functionName)")
        }

        return ""
    }

    public static func callAsync(_ data: String, _ completion: ((_ result: String) -> Void)? = nil) {
        DispatchQueue.global().async {
            let result = call(data)
            completion?(result)
        }
    }

    #if compiler(>=5.7) && canImport(_Concurrency)
        @available(macOS 12, iOS 13, tvOS 13, watchOS 6, *)
        public static func callAsync(_ data: String) async -> String {
            return call(data)
        }
    #endif
}
