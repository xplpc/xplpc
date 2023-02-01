public class DataView: Codable {
    var ptr: UInt
    var size: UInt

    public init(ptr: UInt, size: UInt) {
        self.ptr = ptr
        self.size = size
    }

    public static func createFromByteArray(_ data: inout [UInt8]) -> DataView {
        return DataView(ptr: ByteArrayHelper.getPtrAddress(data), size: UInt(data.count))
    }
}
