public class CallbackList {
    public static var shared: CallbackList = .init()
    private init() {}

    private let lock = DispatchSemaphore(value: 1)
    private var list = [String: (String) -> Void]()

    public func add(key: String, callback: @escaping (String) -> Void) {
        lock.wait()
        defer { lock.signal() }

        list[key] = callback
    }

    public func execute(key: String, data: String) {
        lock.wait()
        defer { lock.signal() }

        if let callback = list[key] {
            list.removeValue(forKey: key)
            callback(data)
        }
    }

    public func count() -> Int {
        lock.wait()
        defer { lock.signal() }

        return list.count
    }
}
