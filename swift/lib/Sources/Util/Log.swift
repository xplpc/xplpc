import Foundation
import os.log

struct Log {
    /// The library writes through a subsystem of its own, so a consumer can filter or silence it and can raise it to debug without a rebuild.
    private static let log = OSLog(subsystem: "XPLPC", category: "xplpc")

    static func d(_ msg: String) {
        os_log("%@", log: log, type: .debug, msg)
    }

    static func e(_ msg: String) {
        os_log("%@", log: log, type: .error, msg)
    }
}
