import Foundation

/// The counter is guarded by the lock below, which is what the unchecked conformance asserts.
class UniqueID: @unchecked Sendable {
    private static let shared: UniqueID = .init()
    private init() {}

    private let lock = NSLock()
    private var counter: UInt = 0

    static func generate() -> String {
        shared.lock.lock()
        defer { shared.lock.unlock() }

        shared.counter &+= 1

        return "SW-\(shared.counter)"
    }
}
