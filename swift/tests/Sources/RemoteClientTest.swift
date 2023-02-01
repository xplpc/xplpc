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

    func reverseResponse(message _: Message, r: Response) {
        r("ok")
    }

    func testLogin() throws {
        let request = Request(
            "sample.login",
            Param("username", "paulo"),
            Param("password", "123456"),
            Param("remember", true)
        )

        RemoteClient.call(request) { response in
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
            RemoteClient.call(request) { response in
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

        RemoteClient.call(request) { (response: Bool?) in
            XCTAssertEqual(nil, response)
        }
    }

    func testReverse() throws {
        MappingList.shared.add(name: "platform.reverse.response", item: MappingItem(target: reverseResponse))

        let request = Request("sample.reverse")

        RemoteClient.call(request) { response in
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

        RemoteClient.call(request) { response in
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

        RemoteClient.call(request) { (r: DataView?) in
            // check response
            XCTAssertNotNil(r)

            guard let dataView = r else {
                return
            }

            // check current values
            var originalData = ByteArrayHelper.createFromDataView(dataView)

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

            RemoteClient.call(request2) { response in
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
}
