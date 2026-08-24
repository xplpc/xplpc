import XCTest
import xplpc

final class CoreTest: XCTestCase {
    func testSetup() {
        XCTAssertEqual(XPLPC.shared.initialized, true)
    }
}
