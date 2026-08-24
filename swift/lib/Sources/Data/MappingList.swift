import Foundation

/// The shared state is guarded by the lock below, which is what the unchecked conformance asserts.
public class MappingList: @unchecked Sendable {
    public static let shared: MappingList = .init()
    private init() {}

    private let lock = NSLock()
    private var list = [String: MappingItem]()

    public func add(name: String, item: MappingItem) {
        lock.lock()
        defer { lock.unlock() }

        list[name] = item
    }

    public func find(_ name: String) -> MappingItem? {
        lock.lock()
        defer { lock.unlock() }

        return list[name]
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

    public func has(_ name: String) -> Bool {
        lock.lock()
        defer { lock.unlock() }

        return list[name] != nil
    }
}
