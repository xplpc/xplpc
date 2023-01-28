public class ByteArrayHelper {
    public static func getPointerAddress(array: [UInt8]) -> UInt {
        var pointerAddress: UInt = 0

        array.withUnsafeBytes { (bytes: UnsafeRawBufferPointer) in
            guard let baseAddress = bytes.baseAddress else {
                return
            }
            pointerAddress = UInt(bitPattern: baseAddress)
        }

        return pointerAddress
    }

    public static func getByteArrayFromPointerAddress(pointerAddress: UInt, count: Int) -> [UInt8] {
        let bytePointer = UnsafeMutableRawPointer(bitPattern: pointerAddress)!
        let bufferPointer = bytePointer.bindMemory(to: UInt8.self, capacity: count)
        let byteArray = Array(UnsafeBufferPointer(start: bufferPointer, count: count))
        return byteArray
    }
}
