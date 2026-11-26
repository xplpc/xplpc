public class Message {
    public init() {}

    var data: [String: AnyCodable?] = [:]

    public func get<T>(_ name: String) -> T? {
        guard let entry = data[name], let stored = entry else {
            return nil
        }

        guard let typed: T = ValueReader.read(stored) else {
            Log.e("[Message : get] Value of \"\(name)\" has another type")
            return nil
        }

        return typed
    }

    public func set(_ name: String, _ value: AnyCodable?) {
        data[name] = value
    }
}
