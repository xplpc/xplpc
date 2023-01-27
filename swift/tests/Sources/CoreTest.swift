import XCTest
@testable import xplpc

final class CoreTest: XCTestCase {
    override func setUpWithError() throws {
        // Put setup code here. This method is called before the invocation of each test method in the class.
    }

    override func tearDownWithError() throws {
        // Put teardown code here. This method is called after the invocation of each test method in the class.
    }

    func testSetup() throws {
        XPLPC.shared.initialize(
            config: Config(serializer: JsonSerializer())
        )

        XCTAssertEqual(XPLPC.shared.initialized, true)
    }
}
