class PlatformProxy: XPLPCNativePlatformProxyImpl {
    static var shared: PlatformProxy = .init()
    override private init() {}

    override func onRemoteClientCall(_ data: String) -> String {
        return ProxyClient.call(data)
    }
}
