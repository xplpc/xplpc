import Foundation

/// The objective-c bridge instantiates this from a framework constructor, which is how the library comes up when the framework loads.
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
