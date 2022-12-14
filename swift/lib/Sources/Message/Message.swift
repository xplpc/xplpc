public class Message {
    var data: [String: AnyCodable?] = [:]

    public func get<T>(_ name: String) -> T? {
        if let d = data[name] {
            if let value = d?.value {
                return value as? T
            }
        }

        return nil
    }

    public func set(_ name: String, _ value: AnyCodable?) {
        data[name] = value
    }

    subscript<T>(_ name: String) -> T? {
        return get(name)
    }
}
