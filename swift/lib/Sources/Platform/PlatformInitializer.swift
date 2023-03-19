import Foundation

@objcMembers public class PlatformInitializer: NSObject {
    override public init() {
        //
    }

    public func initialize() {
        XPLPC.shared.initialize(
            config: Config(serializer: JsonSerializer())
        )
    }
}
