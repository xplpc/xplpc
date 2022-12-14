public class MappingItem {
    private(set) var name: String
    private(set) var target: (_ data: Message) -> Response

    public init(name: String, target: @escaping (_ data: Message) -> Response) {
        self.name = name
        self.target = target
    }
}
