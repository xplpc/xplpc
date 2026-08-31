public class MappingItem {
    private(set) var target: (_ m: Message, _ r: @escaping Response) -> Void

    public init(target: @escaping (_ m: Message, _ r: @escaping Response) -> Void) {
        self.target = target
    }
}
