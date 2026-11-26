public class ByteArrayHelper {
    /// Answers a copy of the bytes the view names, and nothing once the owner released them or when the view names nothing.
    public static func createFromDataView(_ dataView: DataView) -> [UInt8] {
        // A view naming nothing is the empty view every refusal answers with, so it is not looked up.
        guard dataView.buffer != 0, let bytes = ObjCLayerBridge.readBuffer(dataView.buffer) else {
            return []
        }

        return [UInt8](bytes)
    }
}
