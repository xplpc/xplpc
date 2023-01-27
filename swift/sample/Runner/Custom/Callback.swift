import UIKit
import xplpc

class Callback {
    static func batteryLevel(message: Message, r: Response) {
        let level = 100

        let suffix: String = message.get("suffix") ?? ""

        if suffix.isEmpty {
            r("\(level)")
        } else {
            r("\(level)\(suffix)")
        }
    }
}
