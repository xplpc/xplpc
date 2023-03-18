class PlatformProxy: ObjCPlatformProxyImpl {
    static var shared: PlatformProxy = .init()
    override private init() {}

    override func onNativeProxyCallback(_ key: String, _ data: String) {
        CallbackList.shared.execute(key: key, data: data)
    }

    override func onNativeProxyCall(_ key: String, _ data: String) {
        // function name
        let functionName = XPLPC.shared.config.serializer.decodeFunctionName(data)

        if functionName.isEmpty {
            Log.e("[PlatformProxy : call] Function name is empty")
            callNativeProxyCallback(key, "")
            return
        }

        // mapping item
        let mappingItem = MappingList.shared.find(functionName)

        guard let mappingItem else {
            Log.e("[PlatformProxy : call] Mapping not found for function: \(functionName)")
            callNativeProxyCallback(key, "")
            return
        }

        // message
        let message = XPLPC.shared.config.serializer.decodeMessage(data)

        guard let message else {
            Log.e("[PlatformProxy : call] Error when decode message for function: \(functionName)")
            callNativeProxyCallback(key, "")
            return
        }

        // execute
        mappingItem.target(message) { response in
            callNativeProxyCallback(key, XPLPC.shared.config.serializer.encodeFunctionReturnValue(response))
        }
    }

    override func onHasMapping(_ name: String) -> Bool {
        if let _ = MappingList.shared.find(name) {
            return true
        }

        return false
    }

    override func onInitializePlatform() {
        // ignore
    }

    override func onFinalizePlatform() {
        MappingList.shared.clear()
    }
}
