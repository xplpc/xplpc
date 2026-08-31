public class DataView: Codable {
    public let ptr: UInt
    public let size: UInt

    public init(ptr: UInt, size: UInt) {
        self.ptr = ptr
        self.size = size
    }

    public static func withByteArray<T>(_ data: inout [UInt8], _ body: (DataView) throws -> T) rethrows -> T {
        // The address is only stable while the body runs, so the view must never outlive it.

        try data.withUnsafeMutableBytes { buffer in
            guard let baseAddress = buffer.baseAddress else {
                return try body(DataView(ptr: 0, size: 0))
            }

            return try body(DataView(ptr: UInt(bitPattern: baseAddress), size: UInt(buffer.count)))
        }
    }
}
