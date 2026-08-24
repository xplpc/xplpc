public class ByteArrayHelper {
    public static func createFromPtr(ptr: UInt, size: Int) -> [UInt8] {
        guard size > 0, let bytePointer = UnsafeMutableRawPointer(bitPattern: ptr) else {
            return []
        }

        let bufferPointer = bytePointer.bindMemory(to: UInt8.self, capacity: size)

        return Array(UnsafeBufferPointer(start: bufferPointer, count: size))
    }

    public static func createFromDataView(_ dataView: DataView) -> [UInt8] {
        return createFromPtr(ptr: dataView.ptr, size: Int(dataView.size))
    }
}
