import Foundation

/// The shared state is guarded by the locks below, which is what the unchecked conformance asserts.
public class XPLPC: @unchecked Sendable {
    public static let shared: XPLPC = .init()
    private init() {}

    private let lock = NSLock()
    private let initializationLock = NSLock()
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

        // A second caller waits here for the first one to finish rather than returning while the layer is still being attached.
        initializationLock.lock()
        defer { initializationLock.unlock() }

        if initialized {
            return
        }

        guard ObjCLayerBridge.attach(Layer.shared) else {
            Log.e("[XPLPC : initialize] The layer could not be attached")
            return
        }

        // Nothing reads the config while the layer attaches, so it is published with the flag and a failure above leaves nothing behind to retry against.
        lock.lock()
        storedConfig = config
        storedInitialized = true
        lock.unlock()
    }
}
