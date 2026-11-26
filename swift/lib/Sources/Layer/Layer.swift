import Foundation

/// Serves the calls the router hands to the names swift declared.
final class Layer: NSObject, ObjCLayerHost, Sendable {
    static let shared: Layer = .init()
    override private init() {}

    func invoke(_ call: UInt64, request data: String) {
        guard let config = XPLPC.shared.config else {
            Log.e("[Layer : invoke] XPLPC was not initialized")
            ObjCLayerBridge.answer(call, response: "")
            return
        }

        guard let request = config.serializer.decodeRequest(data) else {
            ObjCLayerBridge.answer(call, response: "")
            return
        }

        guard let mappingItem = MappingList.shared.find(request.functionName) else {
            Log.e("[Layer : invoke] Mapping not found for function: \(request.functionName)")
            ObjCLayerBridge.answer(call, response: "")
            return
        }

        mappingItem.target(request.message) { response in
            ObjCLayerBridge.answer(call, response: config.serializer.encodeFunctionReturnValue(response))
        }
    }
}
