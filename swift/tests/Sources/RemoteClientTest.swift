import XCTest

@testable import xplpc

final class RemoteClientTest: XCTestCase {
    override func setUpWithError() throws {
        // Put setup code here. This method is called before the invocation of each test method in the class.
        XPLPC.shared.initialize(
            config: Config(serializer: JsonSerializer())
        )
    }

    override func tearDownWithError() throws {
        // Put teardown code here. This method is called after the invocation of each test method in the class.
    }

    func testLogin() throws {
        let request = Request(
            "sample.login",
            Param("username", "paulo"),
            Param("password", "123456"),
            Param("remember", true)
        )

        let response: String = RemoteClient.call(request) ?? "ERROR"

        XCTAssertEqual("LOGGED-WITH-REMEMBER", response)
    }

    func testLoginCallback() async throws {
        let request = Request(
            "sample.login",
            Param("username", "paulo"),
            Param("password", "123456"),
            Param("remember", true)
        )

        RemoteClient.callAsync(request) { response in
            XCTAssertEqual("LOGGED-WITH-REMEMBER", response)
        }
    }

    #if compiler(>=5.7) && canImport(_Concurrency)
        @available(macOS 12, iOS 13, tvOS 13, watchOS 6, *)
        func testLoginAsync() async throws {
            let request = Request(
                "sample.login",
                Param("username", "paulo"),
                Param("password", "123456"),
                Param("remember", true)
            )

            let response: String = await RemoteClient.callAsync(request) ?? "ERROR"

            XCTAssertEqual("LOGGED-WITH-REMEMBER", response)
        }
    #endif
}
