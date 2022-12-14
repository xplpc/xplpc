public class MappingData {
    public static var shared: MappingData = .init()
    private init() {}

    private var list = [MappingItem]()

    public func add(item: MappingItem) {
        list.append(item)
    }

    public func find(_ name: String) -> MappingItem? {
        return list.first(where: { $0.name == name })
    }
}
