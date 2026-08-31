import Foundation

/// The shared state is guarded by the lock below, which is what the unchecked conformance asserts.
public class XPLPC: @unchecked Sendable {
    public static let shared: XPLPC = .init()
    private init() {}

    private let lock = NSLock()
    private var storedConfig: Config?
    private var storedInitialized = false

    public var initialized: Bool {
        lock.lock()
        defer { lock.unlock() }

        return storedInitialized
    }

    var config: Config? {
        lock.lock()
        defer { lock.unlock() }

        return storedConfig
    }

    public func initialize(config: Config) {
        Log.d("[XPLPC : initialize]")

        lock.lock()

        if storedConfig != nil {
            lock.unlock()
            return
        }

        storedConfig = config
        lock.unlock()

        ObjCPlatformProxyManager.registerProxy(PlatformProxy.shared)

        // The flag is flipped last, so nothing reports itself as ready while the proxy is still being registered.
        lock.lock()
        storedInitialized = true
        lock.unlock()
    }
}
