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
}
