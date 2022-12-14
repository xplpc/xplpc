import Foundation
import os.log

struct Log {
    static func d(_ msg: String, line _: Int = #line, fileName _: String = #file, funcName _: String = #function) {
        #if XPLPC_ENABLE_LOG
            os_log("💙 %@", log: .default, type: .debug, msg)
        #endif
    }

    static func e(_ msg: String, line _: Int = #line, fileName _: String = #file, funcName _: String = #function) {
        os_log("❤️ %@", log: .default, type: .error, msg)
    }
}
