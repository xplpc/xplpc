import XCTest

@testable import xplpc

final class LocalClientTest: XCTestCase {
    override func setUpWithError() throws {
        // Put setup code here. This method is called before the invocation of each test method in the class.
        XPLPC.shared.initialize(
            config: Config(serializer: JsonSerializer())
        )
    }

    override func tearDownWithError() throws {
        // Put teardown code here. This method is called after the invocation of each test method in the class.
    }

    func batteryLevel(message: Message) -> Response {
        let level = 100

        let suffix: String = message.get("suffix") ?? ""

        if suffix.isEmpty {
            return Response("\(level)")
        } else {
            return Response("\(level)\(suffix)")
        }
    }

    func testBatteryLevel() throws {
        MappingData.shared.add(item: MappingItem(name: "platform.battery.level", target: batteryLevel))

        let request = Request("platform.battery.level", Param("suffix", "%"))
        let level: String = LocalClient.call(request) ?? "ERROR"

        XCTAssertEqual("100%", level)
    }

    func testBatteryLevelCallback() async throws {
        MappingData.shared.add(item: MappingItem(name: "platform.battery.level", target: batteryLevel))

        let request = Request("platform.battery.level", Param("suffix", "%"))

        LocalClient.callAsync(request) { level in
            XCTAssertEqual("100%", level)
        }
    }

    #if compiler(>=5.7) && canImport(_Concurrency)
        @available(macOS 12, iOS 13, tvOS 13, watchOS 6, *)
        func testBatteryLevelAsync() async throws {
            MappingData.shared.add(item: MappingItem(name: "platform.battery.level", target: batteryLevel))

            let request = Request("platform.battery.level", Param("suffix", "%"))
            let level: String = await LocalClient.callAsync(request) ?? "ERROR"

            XCTAssertEqual("100%", level)
        }
    #endif
}
