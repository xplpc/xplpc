public class MappingList {
    public static var shared: MappingList = .init()
    private init() {}

    private var list = [String: MappingItem]()

    public func add(name: String, item: MappingItem) {
        list[name] = item
    }

    public func find(_ name: String) -> MappingItem? {
        return list[name]
    }

    public func clear() {
        list.removeAll()
    }

    func has(_ name: String) -> Bool {
        return list.keys.contains(name)
    }
}
