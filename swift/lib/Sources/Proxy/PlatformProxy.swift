class PlatformProxy: XPLPCNativePlatformProxyImpl {
    static var shared: PlatformProxy = .init()
    override private init() {}

    override func onNativeProxyCall(_ key: String, _ data: String) {
        ProxyClient.call(data) { response in
            self.nativeCallProxyCallback(key, response)
        }
    }

    override func callProxyCallback(_ key: String, _ data: String) {
        CallbackList.shared.execute(key: key, data: data)
    }
}
