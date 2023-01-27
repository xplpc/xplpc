public class MappingItem {
    private(set) var target: (_ data: Message, _ r: Response) -> Void

    public init(target: @escaping (_ data: Message, _ r: Response) -> Void) {
        self.target = target
    }
}
