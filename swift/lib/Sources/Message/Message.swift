public class Message {
    var data: [String: AnyCodable?] = [:]

    public func get<T>(_ name: String) -> T? {
        guard let stored = data[name], let value = stored?.value else {
            return nil
        }

        guard let typed = value as? T else {
            Log.e("[Message : get] Value of \"\(name)\" has another type")
            return nil
        }

        return typed
    }

    public func set(_ name: String, _ value: AnyCodable?) {
        data[name] = value
    }
}
