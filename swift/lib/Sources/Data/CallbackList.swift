import Foundation

/// The shared state is guarded by the lock below, which is what the unchecked conformance asserts.
public class CallbackList: @unchecked Sendable {
    public static let shared: CallbackList = .init()
    private init() {}

    private let lock = NSLock()
    private var list = [String: (String) -> Void]()

    public func add(key: String, callback: @escaping (String) -> Void) {
        lock.lock()
        defer { lock.unlock() }

        list[key] = callback
    }

    public func execute(key: String, data: String) {
        lock.lock()
        let callback = list.removeValue(forKey: key)
        lock.unlock()

        callback?(data)
    }

    public func remove(key: String) {
        lock.lock()
        defer { lock.unlock() }

        list.removeValue(forKey: key)
    }

    public func clear() {
        lock.lock()
        defer { lock.unlock() }

        list.removeAll()
    }

    public func count() -> Int {
        lock.lock()
        defer { lock.unlock() }

        return list.count
    }
}
