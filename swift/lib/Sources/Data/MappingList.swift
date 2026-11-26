import Foundation

/// The shared state is guarded by the locks below, which is what the unchecked conformance asserts.
public class MappingList: @unchecked Sendable {
    public static let shared: MappingList = .init()
    private init() {}

    private let lock = NSLock()
    private let registrationLock = NSLock()
    private var list = [String: MappingItem]()

    /// The mapping is recorded before the name is declared, so a call routed the moment the router hears of it already finds it.
    public func add(name: String, item: MappingItem) {
        // Adding and clearing each change two registries, so one never lands between the two steps of the other.
        registrationLock.lock()
        defer { registrationLock.unlock() }

        lock.lock()
        list[name] = item
        lock.unlock()

        ObjCLayerBridge.declare(name)
    }

    public func find(_ name: String) -> MappingItem? {
        lock.lock()
        defer { lock.unlock() }

        return list[name]
    }

    /// The reverse of add, so the router stops handing names over before the mappings behind them are gone.
    public func clear() {
        registrationLock.lock()
        defer { registrationLock.unlock() }

        lock.lock()
        let names = Array(list.keys)
        lock.unlock()

        for name in names {
            ObjCLayerBridge.retract(name)
        }

        lock.lock()
        list.removeAll()
        lock.unlock()
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
