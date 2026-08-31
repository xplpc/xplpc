/// This carries no mutable state of its own and reaches only registries that guard theirs.
class PlatformProxy: ObjCPlatformProxyImpl, @unchecked Sendable {
    static let shared: PlatformProxy = .init()
    override private init() {}

    override func onNativeProxyCallback(_ key: String, _ data: String) {
        CallbackList.shared.execute(key: key, data: data)
    }

    override func onNativeProxyCall(_ key: String, _ data: String) {
        guard let config = XPLPC.shared.config else {
            Log.e("[PlatformProxy : call] XPLPC was not initialized")
            callNativeProxyCallback(key, "")
            return
        }

        guard let request = config.serializer.decodeRequest(data) else {
            callNativeProxyCallback(key, "")
            return
        }

        if request.functionName.isEmpty {
            Log.e("[PlatformProxy : call] Function name is empty")
            callNativeProxyCallback(key, "")
            return
        }

        guard let mappingItem = MappingList.shared.find(request.functionName) else {
            Log.e("[PlatformProxy : call] Mapping not found for function: \(request.functionName)")
            callNativeProxyCallback(key, "")
            return
        }

        mappingItem.target(request.message) { [weak self] response in
            guard let self else {
                Log.e("[PlatformProxy : call] The proxy is gone, so this answer is lost")
                return
            }

            self.callNativeProxyCallback(key, config.serializer.encodeFunctionReturnValue(response))
        }
    }

    override func onHasMapping(_ name: String) -> Bool {
        return MappingList.shared.has(name)
    }

    override func onInitializePlatform() {}

    override func onFinalizePlatform() {
        MappingList.shared.clear()
    }
}
