import Foundation
import os.log

struct Log {
    /// The library writes through a subsystem of its own, so a consumer can filter or silence it and can raise it to debug without a rebuild.
    private static let log = OSLog(subsystem: "XPLPC", category: "xplpc")

    static func d(_ msg: String) {
        os_log("%@", log: log, type: .debug, msg)
    }

    static func e(_ msg: String) {
        // An error line carries names and never data, so it is published rather than redacted from the device log it is needed in.
        os_log("%{public}@", log: log, type: .error, msg)
    }
}
