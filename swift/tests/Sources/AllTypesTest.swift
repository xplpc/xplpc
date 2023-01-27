import XCTest

@testable import xplpc

struct AllTypes: Codable {
    var typeInt8: Int8
    var typeInt16: Int16
    var typeInt32: Int32
    var typeInt64: Int64
    var typeFloat32: Float
    var typeFloat64: Double
    var typeBool: Bool
    var typeOptional: Bool?
    var typeList: [AllTypes]
    var typeMap: [String: String]
    var typeDateTime: Date
    var typeChar: Character
    var typeChar16: Character
    var typeChar32: Character
    var typeWchar: Character
    var typeString: String
}

final class AllTypesTest: XCTestCase {
    override func setUpWithError() throws {
        // Put setup code here. This method is called before the invocation of each test method in the class.
        XPLPC.shared.initialize(
            config: Config(serializer: JsonSerializer())
        )
    }

    override func tearDownWithError() throws {
        // Put teardown code here. This method is called after the invocation of each test method in the class.
    }

    func createItem() -> AllTypes {
        return AllTypes(
            typeInt8: 1,
            typeInt16: 2,
            typeInt32: 3,
            typeInt64: 4,
            typeFloat32: 1.1,
            typeFloat64: 2.2,
            typeBool: true,
            typeOptional: nil,
            typeList: [],
            typeMap: ["item1": "ok"],
            typeDateTime: Date(timeIntervalSince1970: 494_938_800),
            typeChar: "z",
            typeChar16: "®",
            typeChar32: "®",
            typeWchar: "®",
            typeString: "ok"
        )
    }

    func testSingleItem() throws {
        var item = createItem()
        item.typeList.append(item)

        let request = Request("sample.alltypes.single", Param("item", item))

        RemoteClient.call(request) { (r: AllTypes?) in
            XCTAssertNotNil(r)

            if let allTypes = r {
                let mapValue = allTypes.typeMap["item1"]

                XCTAssertEqual(1, allTypes.typeInt8)
                XCTAssertEqual(2, allTypes.typeInt16)
                XCTAssertEqual(3, allTypes.typeInt32)
                XCTAssertEqual(4, allTypes.typeInt64)
                XCTAssertEqual(1.1, allTypes.typeFloat32)
                XCTAssertEqual(2.2, allTypes.typeFloat64)
                XCTAssertEqual(true, allTypes.typeBool)
                XCTAssertEqual(nil, allTypes.typeOptional)
                XCTAssertEqual(1, allTypes.typeList.count)
                XCTAssertEqual("ok", mapValue)
                XCTAssertEqual(494_938_800, allTypes.typeDateTime.timeIntervalSince1970)
                XCTAssertEqual("z", allTypes.typeChar)
                XCTAssertEqual("®", allTypes.typeChar16)
                XCTAssertEqual("®", allTypes.typeChar32)
                XCTAssertEqual("®", allTypes.typeWchar)
                XCTAssertEqual("ok", allTypes.typeString)
            }
        }
    }

    func testMultipleItems() throws {
        var item1 = createItem()
        item1.typeList.append(item1)

        var item2 = createItem()
        item2.typeList.append(item2)

        let list = [item1, item2]

        let request = Request("sample.alltypes.list", Param("items", list))

        RemoteClient.call(request) { (r: [AllTypes]?) in
            XCTAssertEqual(2, r?.count)
        }
    }

    func testSingleItemAsync() async throws {
        var item = createItem()
        item.typeList.append(item)

        let request = Request("sample.alltypes.single", Param("item", item))

        DispatchQueue.global(qos: .background).async {
            RemoteClient.call(request) { (r: AllTypes?) in
                XCTAssertNotNil(r)

                if let allTypes = r {
                    let mapValue = allTypes.typeMap["item1"]

                    XCTAssertEqual(1, allTypes.typeInt8)
                    XCTAssertEqual(2, allTypes.typeInt16)
                    XCTAssertEqual(3, allTypes.typeInt32)
                    XCTAssertEqual(4, allTypes.typeInt64)
                    XCTAssertEqual(1.1, allTypes.typeFloat32)
                    XCTAssertEqual(2.2, allTypes.typeFloat64)
                    XCTAssertEqual(true, allTypes.typeBool)
                    XCTAssertEqual(nil, allTypes.typeOptional)
                    XCTAssertEqual(1, allTypes.typeList.count)
                    XCTAssertEqual("ok", mapValue)
                    XCTAssertEqual(494_938_800, allTypes.typeDateTime.timeIntervalSince1970)
                    XCTAssertEqual("z", allTypes.typeChar)
                    XCTAssertEqual("®", allTypes.typeChar16)
                    XCTAssertEqual("®", allTypes.typeChar32)
                    XCTAssertEqual("®", allTypes.typeWchar)
                    XCTAssertEqual("ok", allTypes.typeString)
                }
            }
        }
    }

    func testMultipleItemsAsync() async throws {
        var item1 = createItem()
        item1.typeList.append(item1)

        var item2 = createItem()
        item2.typeList.append(item2)

        let list = [item1, item2]

        let request = Request("sample.alltypes.list", Param("items", list))

        DispatchQueue.global(qos: .background).async {
            RemoteClient.call(request) { (r: [AllTypes]?) in
                XCTAssertEqual(2, r?.count)
            }
        }
    }
}
