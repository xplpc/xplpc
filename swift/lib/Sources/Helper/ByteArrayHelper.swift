public class ByteArrayHelper {
    public static func getPtrAddress(_ data: [UInt8]) -> UInt {
        var ptr: UInt = 0

        data.withUnsafeBytes { (bytes: UnsafeRawBufferPointer) in
            guard let baseAddress = bytes.baseAddress else {
                return
            }
            ptr = UInt(bitPattern: baseAddress)
        }

        return ptr
    }

    public static func createFromPtr(ptr: UInt, size: Int) -> [UInt8] {
        let bytePointer = UnsafeMutableRawPointer(bitPattern: ptr)!
        let bufferPointer = bytePointer.bindMemory(to: UInt8.self, capacity: size)
        return Array(UnsafeBufferPointer(start: bufferPointer, count: size))
    }
}
