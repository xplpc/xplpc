import XCTest
import xplpc

final class DataViewTest: XPLPCTestCase {
    private let readFailure = "[BufferTable : read]"

    func testTheBodyReadsTheBytesItRegistered() {
        var bytes: [UInt8] = [1, 2, 3, 4]

        let read = DataView.withByteArray(&bytes) { dataView in
            XCTAssertNotEqual(0, dataView.buffer)

            return ByteArrayHelper.createFromDataView(dataView)
        }

        XCTAssertEqual([1, 2, 3, 4], read)
    }

    func testASwiftMappingReadsAViewItReceives() {
        // A parameter that is an object arrives as the decoded document, and the mapping has to be able to read it as the view it is.

        MappingList.shared.add(name: "platform.view.sum", item: MappingItem { message, r in
            guard let view: DataView = message.get("dataView") else {
                r(-1)
                return
            }

            r(AnyCodable(ByteArrayHelper.createFromDataView(view).reduce(0) { $0 + Int($1) }))
        })

        defer { MappingList.shared.clear() }

        var bytes: [UInt8] = [1, 2, 3, 4]

        let sum: Int? = DataView.withByteArray(&bytes) { dataView in
            Client.callSync(Request("platform.view.sum", Param("dataView", dataView)))
        }

        XCTAssertEqual(10, sum)
    }

    func testAViewIsReleasedWhenWithByteArrayReturns() {
        // The view escapes here on purpose, which is the one thing a caller must never do, so the test can see the registration is gone.

        var bytes: [UInt8] = [1, 2, 3, 4]
        let escaped = DataView.withByteArray(&bytes) { $0 }
        var read: [UInt8] = [0]

        let errors = nativeErrors(from: readFailure) {
            read = ByteArrayHelper.createFromDataView(escaped)
        }

        XCTAssertNotEqual(0, escaped.buffer)
        XCTAssertTrue(read.isEmpty)
        XCTAssertEqual(1, errors)
        XCTAssertFalse(ObjCLayerBridge.releaseBuffer(escaped.buffer))
    }

    func testAViewNamingNothingIsNotLookedUp() {
        var read: [UInt8] = [0]

        let errors = nativeErrors(from: readFailure) {
            read = ByteArrayHelper.createFromDataView(DataView(buffer: 0))
        }

        XCTAssertTrue(read.isEmpty)
        XCTAssertEqual(0, errors)
    }

    func testAHandleNobodyRegisteredReadsAsNothing() {
        // The slot sits just below the limit a handle can reach, so nothing in this process ever registered it.

        let unregistered = DataView(buffer: (UInt64(2_000_000) << 32) | 1)
        var read: [UInt8] = [0]

        let errors = nativeErrors(from: readFailure) {
            read = ByteArrayHelper.createFromDataView(unregistered)
        }

        XCTAssertTrue(read.isEmpty)
        XCTAssertEqual(1, errors)
    }

    func testAnEmptyArrayNamesNothing() {
        var bytes: [UInt8] = []

        let buffer = DataView.withByteArray(&bytes) { $0.buffer }

        XCTAssertEqual(0, buffer)
    }

    func testOnlyTheLayerThatRegisteredABufferReleasesIt() {
        // The c++ mapping registered this buffer, so swift is refused and the bytes stay readable.

        let dataView: DataView? = Client.callSync(Request("sample.dataview"))

        guard let view = dataView else {
            XCTFail("The c++ mapping answered no view")
            return
        }

        XCTAssertFalse(ObjCLayerBridge.releaseBuffer(view.buffer))
        XCTAssertEqual(16, ByteArrayHelper.createFromDataView(view).count)
    }

    func testAValueThatIsNotAHandleNamesNothingAndLeavesTheRestReadable() {
        struct Carrier: Decodable {
            let view: DataView
            let label: String
        }

        let refused = [
            #"{"buffer":true}"#,
            #"{"buffer":2.5}"#,
            #"{"buffer":-1}"#,
            #"{"buffer":"1"}"#,
            #"{"buffer":null}"#,
            #"{"ptr":1,"size":4}"#,
            "7",
        ]

        for value in refused {
            let carrier: Carrier? = JsonSerializer().decodeFunctionReturnValue(#"{"r":{"view":\#(value),"label":"kept"}}"#)

            XCTAssertEqual(0, carrier?.view.buffer, value)
            XCTAssertEqual("kept", carrier?.label, value)
        }

        let accepted: Carrier? = JsonSerializer().decodeFunctionReturnValue(#"{"r":{"view":{"buffer":18446744073709551615},"label":"kept"}}"#)

        XCTAssertEqual(UInt64.max, accepted?.view.buffer)
    }
}
