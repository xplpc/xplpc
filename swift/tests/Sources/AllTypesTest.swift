import XCTest
import xplpc

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
    var typeChar: CodePoint
    var typeChar16: CodePoint
    var typeChar32: CodePoint
    var typeWchar: CodePoint
    var typeString: String
}

final class AllTypesTest: XCTestCase {
    func createItem() -> AllTypes {
        // The edges are what a serializer loses, since a small value round trips even when the format cannot carry the type.
        return AllTypes(
            typeInt8: Int8.min,
            typeInt16: Int16.min,
            typeInt32: Int32.min,
            typeInt64: 9_007_199_254_740_993,
            typeFloat32: 0.1,
            typeFloat64: 0.1,
            typeBool: true,
            typeOptional: nil,
            typeList: [],
            typeMap: ["item1": "ok"],
            typeDateTime: Date(timeIntervalSince1970: 494_938.8),
            typeChar: "z",
            typeChar16: "®",
            typeChar32: "®",
            typeWchar: "®",
            typeString: "ok"
        )
    }

    func verifyCall<T: Decodable>(_ request: Request, _ block: @escaping (T?) -> Void) {
        // A callback that never runs would leave every check unexecuted and the test green, so the test waits for it.

        let answered = expectation(description: "answered")

        Client.call(request) { (response: T?) in
            block(response)
            answered.fulfill()
        }

        wait(for: [answered], timeout: 5)
    }

    func verifyCallFromBackground<T: Decodable>(_ request: Request, _ block: @escaping (T?) -> Void) {
        // The call is made from a background thread and the test still has to wait for the answer on its own.

        let answered = expectation(description: "answered")

        DispatchQueue.global(qos: .userInitiated).async {
            Client.call(request) { (response: T?) in
                block(response)
                answered.fulfill()
            }
        }

        wait(for: [answered], timeout: 5)
    }

    func testSingleItem() {
        var item = createItem()
        item.typeList.append(item)

        let request = Request("sample.alltypes.single", Param("item", item))

        verifyCall(request) { (r: AllTypes?) in
            XCTAssertNotNil(r)

            if let allTypes = r {
                let mapValue = allTypes.typeMap["item1"]

                XCTAssertEqual(Int8.min, allTypes.typeInt8)
                XCTAssertEqual(Int16.min, allTypes.typeInt16)
                XCTAssertEqual(Int32.min, allTypes.typeInt32)
                XCTAssertEqual(9_007_199_254_740_993, allTypes.typeInt64)
                XCTAssertEqual(0.1, allTypes.typeFloat32)
                XCTAssertEqual(0.1, allTypes.typeFloat64)
                XCTAssertEqual(true, allTypes.typeBool)
                XCTAssertEqual(nil, allTypes.typeOptional)
                XCTAssertEqual(1, allTypes.typeList.count)
                XCTAssertEqual("ok", mapValue)
                XCTAssertEqual(494_938_800, Int64((allTypes.typeDateTime.timeIntervalSince1970 * 1000).rounded()))
                XCTAssertEqual("z", allTypes.typeChar)
                XCTAssertEqual("®", allTypes.typeChar16)
                XCTAssertEqual("®", allTypes.typeChar32)
                XCTAssertEqual("®", allTypes.typeWchar)
                XCTAssertEqual("ok", allTypes.typeString)
            }
        }
    }

    func testMultipleItems() {
        var item1 = createItem()
        item1.typeList.append(item1)

        var item2 = createItem()
        item2.typeList.append(item2)

        let list = [item1, item2]

        let request = Request("sample.alltypes.list", Param("items", list))

        verifyCall(request) { (r: [AllTypes]?) in
            XCTAssertEqual(2, r?.count)
        }
    }

    func testSingleItemAsync() {
        var item = createItem()
        item.typeList.append(item)

        let request = Request("sample.alltypes.single", Param("item", item))

        verifyCallFromBackground(request) { (r: AllTypes?) in
            XCTAssertNotNil(r)

            if let allTypes = r {
                let mapValue = allTypes.typeMap["item1"]

                XCTAssertEqual(Int8.min, allTypes.typeInt8)
                XCTAssertEqual(Int16.min, allTypes.typeInt16)
                XCTAssertEqual(Int32.min, allTypes.typeInt32)
                XCTAssertEqual(9_007_199_254_740_993, allTypes.typeInt64)
                XCTAssertEqual(0.1, allTypes.typeFloat32)
                XCTAssertEqual(0.1, allTypes.typeFloat64)
                XCTAssertEqual(true, allTypes.typeBool)
                XCTAssertEqual(nil, allTypes.typeOptional)
                XCTAssertEqual(1, allTypes.typeList.count)
                XCTAssertEqual("ok", mapValue)
                XCTAssertEqual(494_938_800, Int64((allTypes.typeDateTime.timeIntervalSince1970 * 1000).rounded()))
                XCTAssertEqual("z", allTypes.typeChar)
                XCTAssertEqual("®", allTypes.typeChar16)
                XCTAssertEqual("®", allTypes.typeChar32)
                XCTAssertEqual("®", allTypes.typeWchar)
                XCTAssertEqual("ok", allTypes.typeString)
            }
        }
    }

    func testMultipleItemsAsync() {
        var item1 = createItem()
        item1.typeList.append(item1)

        var item2 = createItem()
        item2.typeList.append(item2)

        let list = [item1, item2]

        let request = Request("sample.alltypes.list", Param("items", list))

        verifyCallFromBackground(request) { (r: [AllTypes]?) in
            XCTAssertEqual(2, r?.count)
        }
    }
}
