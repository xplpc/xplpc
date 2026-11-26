import XCTest
import xplpc

final class CoreTest: XPLPCTestCase {
    func testSetup() {
        XCTAssertEqual(XPLPC.shared.initialized, true)
    }

    func testInitializingAgainFromManyThreadsChangesNothing() {
        // The framework constructor already brought the library up, so every one of these calls has to leave the config it published in place.

        DispatchQueue.concurrentPerform(iterations: 16) { _ in
            XPLPC.shared.initialize(config: Config(serializer: ReplacingSerializer()))
        }

        XCTAssertTrue(XPLPC.shared.initialized)
        XCTAssertTrue(Request("sample.version").data.contains(#""f":"sample.version""#))
    }
}

private final class ReplacingSerializer: Serializer {
    func encodeRequest(_: String, _: [Param]) -> String {
        "replaced"
    }

    func decodeFunctionReturnValue<T>(_: String) -> T? {
        nil
    }

    /// A serializer written outside the library builds the request it decodes, which is what makes the protocol implementable by a consumer.
    func decodeRequest(_: String) -> DecodedRequest? {
        let message = Message()
        message.set("replaced", AnyCodable(true))

        return DecodedRequest(functionName: "replaced", message: message)
    }

    func encodeFunctionReturnValue(_: ResponseData?) -> String {
        "replaced"
    }
}
