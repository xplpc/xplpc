public class ProxyClient {
    public static func call(_ data: String, callback: ((String) -> Void)?) {
        // function name
        let functionName = XPLPC.shared.config.serializer.decodeFunctionName(data)

        if functionName.isEmpty {
            Log.e("[ProxyClient : call] Function name is empty")
            callback?("")
            return
        }

        // mapping item
        let mappingItem = MappingList.shared.find(functionName)

        guard let mappingItem else {
            Log.e("[ProxyClient : call] Mapping not found for function: \(functionName)")
            callback?("")
            return
        }

        // message
        let message = XPLPC.shared.config.serializer.decodeMessage(data)

        guard let message else {
            Log.e("[ProxyClient : call] Error when decode message for function: \(functionName)")
            callback?("")
            return
        }

        // execute
        mappingItem.target(message) { response in
            callback?(XPLPC.shared.config.serializer.encodeFunctionReturnValue(response))
        }
    }
}
