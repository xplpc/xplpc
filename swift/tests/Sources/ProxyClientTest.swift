import XCTest

@testable import xplpc

final class ProxyClientTest: XCTestCase {
    override func setUpWithError() throws {
        // Put setup code here. This method is called before the invocation of each test method in the class.
        XPLPC.shared.initialize(
            config: Config(serializer: JsonSerializer())
        )
    }

    override func tearDownWithError() throws {
        // Put teardown code here. This method is called after the invocation of each test method in the class.
    }

    func batteryLevel(message: Message, r: Response) {
        let level = 100

        let suffix: String = message.get("suffix") ?? ""

        if suffix.isEmpty {
            r("\(level)")
        } else {
            r("\(level)\(suffix)")
        }
    }

    func testBatteryLevel() throws {
        MappingList.shared.add(name: "platform.battery.level", item: MappingItem(target: batteryLevel))

        let request = Request("platform.battery.level", Param("suffix", "%"))

        ProxyClient.call(request.data) { response in
            XCTAssertEqual("{\"r\":\"100%\"}", response)
        }
    }

    func testBatteryLevelAsync() async throws {
        MappingList.shared.add(name: "platform.battery.level", item: MappingItem(target: batteryLevel))

        let request = Request("platform.battery.level", Param("suffix", "%"))

        DispatchQueue.global(qos: .background).async {
            ProxyClient.call(request.data) { response in
                XCTAssertEqual("{\"r\":\"100%\"}", response)
            }
        }
    }
}
