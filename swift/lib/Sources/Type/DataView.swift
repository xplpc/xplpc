/// Names a buffer registered by whoever owns the bytes, which is all that crosses the wire, so a view can never point a reader at memory nobody registered.
public final class DataView: Codable, Sendable {
    public let buffer: UInt64

    public init(buffer: UInt64) {
        self.buffer = buffer
    }

    /// Anything that is not a handle names no buffer, so one refused value leaves the rest of the document readable.
    public init(from decoder: Decoder) {
        do {
            let container = try decoder.container(keyedBy: CodingKeys.self)
            buffer = try container.decode(UInt64.self, forKey: .buffer)
        } catch let e {
            Log.e("[DataView : init] Value is not a data view")
            Log.d("[DataView : init] Value is not a data view: \(e.localizedDescription)")

            buffer = 0
        }
    }

    /// Registers the bytes for as long as the body runs and releases them when it returns, so the view must never outlive it.
    public static func withByteArray<T>(_ data: inout [UInt8], _ body: (DataView) throws -> T) rethrows -> T {
        try data.withUnsafeMutableBytes { bytes in
            guard let address = bytes.baseAddress, !bytes.isEmpty else {
                return try body(DataView(buffer: 0))
            }

            let buffer = ObjCLayerBridge.registerBuffer(address, size: UInt(bytes.count))

            defer {
                if buffer != 0 {
                    ObjCLayerBridge.releaseBuffer(buffer)
                }
            }

            return try body(DataView(buffer: buffer))
        }
    }
}
