public class XPLPC {
    public static var shared: XPLPC = .init()

    private(set) var initialized = false
    private(set) var config: Config!

    private init() {}

    public func initialize(config: Config) {
        Log.d("[XPLPC : initialize]")

        if initialized {
            return
        }

        initialized = true
        self.config = config

        ObjCPlatformProxyManager.initialize(PlatformProxy.shared)
    }
}
