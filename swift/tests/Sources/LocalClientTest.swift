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

        LocalClient.call(request) { response in
            XCTAssertEqual("100%", response)
        }
    }

    func testBatteryLevelAsync() throws {
        MappingList.shared.add(name: "platform.battery.level", item: MappingItem(target: batteryLevel))

        let request = Request("platform.battery.level", Param("suffix", "%"))

        DispatchQueue.global(qos: .background).async {
            LocalClient.call(request) { response in
                XCTAssertEqual("100%", response)
            }
        }
    }

    func testBatteryLevelInvalidCast() throws {
        MappingList.shared.add(name: "platform.battery.level", item: MappingItem(target: batteryLevel))

        let request = Request("platform.battery.level", Param("suffix", "%"))

        LocalClient.call(request) { (response: Bool?) in
            XCTAssertEqual(nil, response)
        }
    }
}
