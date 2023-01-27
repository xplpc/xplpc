public class LocalClient {
    public static func call<T: Decodable>(_ request: Request, _ callback: ((_ result: T?) -> Void)? = nil) {
        let data = request.data

        // function name
        let functionName = XPLPC.shared.config.serializer.decodeFunctionName(data)

        if functionName.isEmpty {
            Log.e("[LocalClient : call] Function name is empty")
            callback?(nil)
            return
        }

        // mapping item
        let mappingItem = MappingList.shared.find(functionName)

        guard let mappingItem else {
            Log.e("[LocalClient : call] Mapping not found for function: \(functionName)")
            callback?(nil)
            return
        }

        // message
        guard let message = XPLPC.shared.config.serializer.decodeMessage(data) else {
            Log.e("[LocalClient : call] Error when decode message for function: \(functionName)")
            callback?(nil)
            return
        }

        // execute
        mappingItem.target(message) { response in
            guard let response else {
                Log.e("[LocalClient : call] Error when decode value for function: \(functionName)")
                callback?(nil)
                return
            }

            callback?(response.value as? T)
        }
    }
}
