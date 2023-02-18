public class MappingItem {
    private(set) var target: (_ m: Message, _ r: Response) -> Void

    public init(target: @escaping (_ m: Message, _ r: Response) -> Void) {
        self.target = target
    }
}
