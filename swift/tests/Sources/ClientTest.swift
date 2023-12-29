import XCTest

@testable import xplpc

final class ClientTest: XCTestCase {
    override func setUpWithError() throws {
        // Put setup code here. This method is called before the invocation of each test method in the class.
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

    func reverseResponse(message _: Message, r: Response) {
        r("ok")
    }

    func testBatteryLevel() throws {
        MappingList.shared.add(name: "platform.battery.level", item: MappingItem(target: batteryLevel))

        let request = Request("platform.battery.level", Param("suffix", "%"))

        Client.call(request) { response in
            XCTAssertEqual("100%", response)
        }
    }

    func testBatteryLevelAsync() throws {
        MappingList.shared.add(name: "platform.battery.level", item: MappingItem(target: batteryLevel))

        let request = Request("platform.battery.level", Param("suffix", "%"))

        DispatchQueue.global(qos: .background).async {
            Client.call(request) { response in
                XCTAssertEqual("100%", response)
            }
        }
    }

    #if compiler(>=5.5) && canImport(_Concurrency)
        func testBatteryLevelCallAsync() async throws {
            MappingList.shared.add(name: "platform.battery.level", item: MappingItem(target: batteryLevel))

            let request = Request("platform.battery.level", Param("suffix", "%"))

            if let response: String? = await Client.callAsync(request) {
                XCTAssertEqual("100%", response)
            }
        }
    #endif

    func testBatteryLevelInvalidCast() throws {
        MappingList.shared.add(name: "platform.battery.level", item: MappingItem(target: batteryLevel))

        let request = Request("platform.battery.level", Param("suffix", "%"))

        Client.call(request) { (response: Bool?) in
            XCTAssertEqual(nil, response)
        }
    }

    func testLogin() throws {
        let request = Request(
            "sample.login",
            Param("username", "paulo"),
            Param("password", "123456"),
            Param("remember", true)
        )

        Client.call(request) { response in
            XCTAssertEqual("LOGGED-WITH-REMEMBER", response)
        }
    }

    func testLoginAsync() throws {
        let request = Request(
            "sample.login",
            Param("username", "paulo"),
            Param("password", "123456"),
            Param("remember", true)
        )

        DispatchQueue.global(qos: .background).async {
            Client.call(request) { response in
                XCTAssertEqual("LOGGED-WITH-REMEMBER", response)
            }
        }
    }

    func testLoginInvalidCast() throws {
        let request = Request(
            "sample.login",
            Param("username", "paulo"),
            Param("password", "123456"),
            Param("remember", true)
        )

        Client.call(request) { (response: Bool?) in
            XCTAssertEqual(nil, response)
        }
    }

    func testReverse() throws {
        MappingList.shared.add(name: "platform.reverse.response", item: MappingItem(target: reverseResponse))

        let request = Request("sample.reverse")

        Client.call(request) { response in
            XCTAssertEqual("response-is-ok", response)
        }
    }

    func grayscaleImageWithDataView() throws {
        var data: [UInt8] = [
            // red pixel
            255, 0, 0, 255,
            // green pixel
            0, 255, 0, 255,
            // blue pixel
            0, 0, 255, 255,
            // transparent pixel
            0, 0, 0, 0,
        ]

        let dataView = DataView.createFromByteArray(&data)

        let request = Request(
            "sample.image.grayscale.dataview",
            Param("dataView", dataView)
        )

        Client.call(request) { response in
            XCTAssertEqual("OK", response)

            XCTAssertEqual(16, data.count)
            XCTAssertEqual(data[0], 85)
            XCTAssertEqual(data[4], 85)
            XCTAssertEqual(data[8], 85)
            XCTAssertEqual(data[12], 0)
        }
    }

    func testDataView() throws {
        // get data view
        let request = Request("sample.dataview")

        Client.call(request) { (r: DataView?) in
            // check response
            XCTAssertNotNil(r)

            guard let dataView = r else {
                return
            }

            // check current values
            let originalData = ByteArrayHelper.createFromDataView(dataView)

            XCTAssertEqual(16, dataView.size)
            XCTAssertEqual(originalData[0], 255)
            XCTAssertEqual(originalData[5], 255)
            XCTAssertEqual(originalData[10], 255)
            XCTAssertEqual(originalData[12], 0)

            // send original data and check modified data
            let request2 = Request(
                "sample.image.grayscale.dataview",
                Param("dataView", dataView)
            )

            Client.call(request2) { response in
                XCTAssertEqual("OK", response)

                let processedData = ByteArrayHelper.createFromDataView(dataView)

                // check copied values
                XCTAssertEqual(16, processedData.count)
                XCTAssertEqual(processedData[0], 85)
                XCTAssertEqual(processedData[4], 85)
                XCTAssertEqual(processedData[8], 85)
                XCTAssertEqual(processedData[12], 0)

                // check original values again
                XCTAssertEqual(16, dataView.size)
                XCTAssertEqual(originalData[0], 255)
                XCTAssertEqual(originalData[5], 255)
                XCTAssertEqual(originalData[10], 255)
                XCTAssertEqual(originalData[12], 0)
            }
        }
    }

    func testBatteryLevelFromString() throws {
        MappingList.shared.add(name: "platform.battery.level", item: MappingItem(target: batteryLevel))

        let request = Request("platform.battery.level", Param("suffix", "%"))

        Client.call(request.data) { response in
            XCTAssertEqual("{\"r\":\"100%\"}", response)
        }
    }

    func testBatteryLevelAsyncFromString() async throws {
        MappingList.shared.add(name: "platform.battery.level", item: MappingItem(target: batteryLevel))

        let request = Request("platform.battery.level", Param("suffix", "%"))

        DispatchQueue.global(qos: .background).async {
            Client.call(request.data) { response in
                XCTAssertEqual("{\"r\":\"100%\"}", response)
            }
        }
    }
}
