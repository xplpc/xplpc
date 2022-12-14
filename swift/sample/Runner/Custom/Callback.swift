import UIKit
import xplpc

class Callback {
    static func batteryLevel(message: Message) -> Response {
        let level = Int(UIDevice.current.batteryLevel * 100)
        let suffix: String = message.get("suffix") ?? ""

        if suffix.isEmpty {
            return Response("\(level)")
        } else {
            return Response("\(level)\(suffix)")
        }
    }
}
