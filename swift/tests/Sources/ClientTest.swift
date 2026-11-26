import XCTest
import xplpc

final class ClientTest: XPLPCTestCase {
    func mismatchedType(message: Message, r: Response) {
        let asString: String? = message.get("value")
        let asInt: Int? = message.get("value")

        r("\(asString ?? "nil")/\(asInt ?? -1)")
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

    func testBatteryLevel() {
        MappingList.shared.add(name: "platform.battery.level", item: MappingItem(target: batteryLevel))

        let request = Request("platform.battery.level", Param("suffix", "%"))

        verifyCall(request) { (response: String?) in
            XCTAssertEqual("100%", response)
        }
    }

    func testBatteryLevelAsync() {
        MappingList.shared.add(name: "platform.battery.level", item: MappingItem(target: batteryLevel))

        let request = Request("platform.battery.level", Param("suffix", "%"))

        let answered = expectation(description: "answered")

        DispatchQueue.global(qos: .userInitiated).async {
            Client.call(request) { (response: String?) in
                XCTAssertEqual("100%", response)
                answered.fulfill()
            }
        }

        wait(for: [answered], timeout: 5)
    }

    @MainActor
    func testCallAsyncGivesTheSchedulerATurnBeforeAnswering() async {
        // A call site has to behave the same whether the mapping is quick or slow, so the scheduler is reached even when the answer is already there.

        let box = OrderBox()

        Task { @MainActor in box.order.append("scheduler") }

        let _: String? = await Client.callAsync(Request("sample.version"))
        box.order.append("after await")

        XCTAssertEqual(["scheduler", "after await"], box.order)
    }

    func testBatteryLevelCallAsync() async {
        MappingList.shared.add(name: "platform.battery.level", item: MappingItem(target: batteryLevel))

        let request = Request("platform.battery.level", Param("suffix", "%"))

        let response: String? = await Client.callAsync(request)

        XCTAssertEqual("100%", response)
    }

    func testBatteryLevelInvalidCast() {
        MappingList.shared.add(name: "platform.battery.level", item: MappingItem(target: batteryLevel))

        let request = Request("platform.battery.level", Param("suffix", "%"))

        verifyCall(request) { (response: Bool?) in
            XCTAssertEqual(nil, response)
        }
    }

    func testLogin() {
        let request = Request(
            "sample.login",
            Param("username", "paulo"),
            Param("password", "123456"),
            Param("remember", true)
        )

        verifyCall(request) { (response: String?) in
            XCTAssertEqual("LOGGED-WITH-REMEMBER", response)
        }
    }

    func testLoginAsync() {
        let request = Request(
            "sample.login",
            Param("username", "paulo"),
            Param("password", "123456"),
            Param("remember", true)
        )

        let answered = expectation(description: "answered")

        DispatchQueue.global(qos: .userInitiated).async {
            Client.call(request) { (response: String?) in
                XCTAssertEqual("LOGGED-WITH-REMEMBER", response)
                answered.fulfill()
            }
        }

        wait(for: [answered], timeout: 5)
    }

    func testLoginInvalidCast() {
        let request = Request(
            "sample.login",
            Param("username", "paulo"),
            Param("password", "123456"),
            Param("remember", true)
        )

        verifyCall(request) { (response: Bool?) in
            XCTAssertEqual(nil, response)
        }
    }

    func testReverse() {
        MappingList.shared.add(name: "platform.reverse.response", item: MappingItem(target: reverseResponse))

        let request = Request("sample.reverse")

        verifyCall(request) { (response: String?) in
            XCTAssertEqual("response-is-ok", response)
        }
    }

    func testGrayscaleImageWithDataViewChangesTheCallersArrayInPlace() {
        var data: [UInt8] = [
            255, 0, 0, 255,
            0, 255, 0, 255,
            0, 0, 255, 255,
            0, 0, 0, 0,
        ]

        DataView.withByteArray(&data) { dataView in
            let request = Request(
                "sample.image.grayscale.dataview",
                Param("dataView", dataView)
            )

            verifyCall(request) { (response: String?) in
                XCTAssertEqual("OK", response)
            }
        }

        XCTAssertEqual([85, 85, 85, 255, 85, 85, 85, 255, 85, 85, 85, 255, 0, 0, 0, 0], data)
    }

    func testDataViewAnsweredByCxxIsReadable() {
        let request = Request("sample.dataview")

        verifyCall(request) { (r: DataView?) in
            guard let dataView = r else {
                XCTFail("The c++ mapping answered no view")
                return
            }

            let originalData = ByteArrayHelper.createFromDataView(dataView)

            XCTAssertEqual([255, 0, 0, 255, 0, 255, 0, 255, 0, 0, 255, 255, 0, 0, 0, 0], originalData)

            let request2 = Request(
                "sample.image.grayscale.dataview",
                Param("dataView", dataView)
            )

            self.verifyCall(request2) { (response: String?) in
                XCTAssertEqual("OK", response)

                // The c++ buffer changed in place, while the copy read before the call kept what it was.
                let processedData = ByteArrayHelper.createFromDataView(dataView)

                XCTAssertEqual([85, 85, 85, 255, 85, 85, 85, 255, 85, 85, 85, 255, 0, 0, 0, 0], processedData)
                XCTAssertEqual(255, originalData[0])
            }
        }
    }

    func testBatteryLevelFromString() {
        MappingList.shared.add(name: "platform.battery.level", item: MappingItem(target: batteryLevel))

        let request = Request("platform.battery.level", Param("suffix", "%"))

        verifyCall(request.data) { response in
            XCTAssertEqual("{\"r\":\"100%\"}", response)
        }
    }

    func testBatteryLevelAsyncFromString() {
        MappingList.shared.add(name: "platform.battery.level", item: MappingItem(target: batteryLevel))

        let request = Request("platform.battery.level", Param("suffix", "%"))

        let answered = expectation(description: "answered")

        DispatchQueue.global(qos: .userInitiated).async {
            Client.call(request.data) { response in
                XCTAssertEqual("{\"r\":\"100%\"}", response)
                answered.fulfill()
            }
        }

        wait(for: [answered], timeout: 5)
    }

    func testBatteryLevelCallAsyncFromString() async {
        MappingList.shared.add(name: "platform.battery.level", item: MappingItem(target: batteryLevel))

        let request = Request("platform.battery.level", Param("suffix", "%"))
        let response: String = await Client.callAsync(request.data)

        XCTAssertEqual("{\"r\":\"100%\"}", response)
    }

    func testEchoesEveryEncodingWidth() {
        // A string has to survive every width utf8 can encode it in, on the way out and on the way back.

        let samples = [
            "plain ascii",
            "caf\u{00e9} na\u{00ef}ve",
            "\u{4e2d}\u{6587}\u{30c6}\u{30b9}\u{30c8}",
            "\u{1F600}\u{1F468}\u{200D}\u{1F469}",
            "mixed \u{00e9} \u{4e2d} \u{1F600} end",
        ]

        for sample in samples {
            verifyCall(Request("sample.echo", Param("value", sample))) { (response: String?) in
                XCTAssertEqual(sample, response)
            }
        }
    }

    func testLargeDataViewCrossesWithoutTruncating() {
        // A buffer far past what a small integer holds has to cross without being truncated anywhere on the way, so its last pixel is the one that has to change.

        let size = 4 * 1024 * 1024
        var bytes = [UInt8](repeating: 100, count: size)
        bytes.replaceSubrange(size - 4 ..< size, with: [30, 60, 90, 255])

        DataView.withByteArray(&bytes) { dataView in
            let request = Request("sample.image.grayscale.dataview", Param("dataView", dataView))

            verifyCall(request) { (response: String?) in
                XCTAssertEqual("OK", response)
            }
        }

        XCTAssertEqual(100, bytes[0])
        XCTAssertEqual([60, 60, 60, 255], Array(bytes[size - 4 ..< size]))
    }

    func testCoreMappingAnswersFromItsOwnThread() {
        // The mapping answers after the call returned, from a thread it created, and still has to reach swift.

        let answered = expectation(description: "answered")

        Client.call(Request("sample.async")) { (_: String?) in
            answered.fulfill()
        }

        wait(for: [answered], timeout: 5)
    }

    func testNestedHostMappingAnsweringLaterStillReachesTheOuterCaller() {
        let box = ResponderBox()

        MappingList.shared.add(name: "platform.reverse.response", item: MappingItem { _, r in box.keep(r) })

        let answered = expectation(description: "nested")
        var response: String?

        Client.call(Request("sample.reverse")) { (result: String?) in
            response = result
            answered.fulfill()
        }

        XCTAssertNil(response)
        XCTAssertNotNil(box.responder)

        DispatchQueue.global(qos: .userInitiated).async {
            box.responder?("ok")
        }

        wait(for: [answered], timeout: 5)

        XCTAssertEqual("response-is-ok", response)
    }

    func testHostMappingAnswersAfterTheCallReturned() {
        let box = ResponderBox()

        MappingList.shared.add(name: "platform.deferred.answer", item: MappingItem { _, r in box.keep(r) })

        let answered = expectation(description: "deferred")
        var response: String?

        Client.call(Request("platform.deferred.answer")) { (result: String?) in
            response = result
            answered.fulfill()
        }

        XCTAssertNil(response)

        DispatchQueue.global(qos: .userInitiated).async {
            box.responder?("deferred")
        }

        wait(for: [answered], timeout: 5)

        XCTAssertEqual("deferred", response)
    }

    func testMappingReadingAMismatchedTypeGetsNothing() {
        // Reading a value as a type it does not hold answers nothing rather than a value read the wrong way.

        MappingList.shared.add(name: "platform.mismatched.type", item: MappingItem(target: mismatchedType))

        let request = Request("platform.mismatched.type", Param("value", 10))

        verifyCall(request) { (response: String?) in
            XCTAssertEqual("nil/10", response)
        }
    }

    func testUnknownFunctionAnswersEmptyOnce() {
        // Nothing owns this name, so the router answers the caller with the empty value itself, and only once.

        verifyCall(Request("not.found")) { (response: String?) in
            XCTAssertNil(response)
        }
    }

    func testANameDeclaredWithoutALocalMappingIsAnsweredEmpty() {
        // The router routes by what was declared, so a name swift declared and never recorded reaches the swift layer and finds nothing there to run.

        ObjCLayerBridge.declare("platform.declared.only")
        defer { ObjCLayerBridge.retract("platform.declared.only") }

        verifyCall(#"{"f":"platform.declared.only","p":[]}"#) { response in
            XCTAssertEqual("", response)
        }

        XCTAssertEqual(0, pendingCalls())
    }

    func testCallSyncAnswersTheValueFromAnInlineMapping() {
        // A mapping that answers before it returns has already resolved the call, so the value is there to be read.

        MappingList.shared.add(name: "platform.battery.level", item: MappingItem(target: batteryLevel))

        let request = Request("platform.battery.level", Param("suffix", "%"))
        let response: String? = Client.callSync(request)

        XCTAssertEqual("100%", response)
    }

    func testCallSyncAnswersTheValueFromAnInlineNativeMapping() {
        let request = Request(
            "sample.login",
            Param("username", "paulo"),
            Param("password", "123456"),
            Param("remember", true)
        )

        let response: String? = Client.callSync(request)

        XCTAssertEqual("LOGGED-WITH-REMEMBER", response)
    }

    func testCallSyncAnswersEmptyWhenTheMappingDefers() {
        // A mapping that answers later cannot be read synchronously, so the call is cancelled and the answer it produces later resolves nothing.

        let box = ResponderBox()

        MappingList.shared.add(name: "platform.deferred.answer", item: MappingItem { _, r in box.keep(r) })

        let response: String? = Client.callSync(Request("platform.deferred.answer"))

        XCTAssertNil(response)

        // The mapping answers a call that is already gone, which resolves nothing.
        box.responder?("deferred")
    }

    func testCallSyncAnswersEmptyWhenNothingOwnsTheFunction() {
        let response: String? = Client.callSync(Request("not.found"))

        XCTAssertNil(response)
    }

    func testCallSyncAnswersNothingForAMismatchedType() {
        let request = Request(
            "sample.login",
            Param("username", "paulo"),
            Param("password", "123456"),
            Param("remember", true)
        )

        let response: Bool? = Client.callSync(request)

        XCTAssertNil(response)
    }

    func testCallSyncAnswersTheDocumentFromAString() {
        let request = "{\"f\":\"sample.login\",\"p\":[{\"n\":\"username\",\"v\":\"paulo\"},"
            + "{\"n\":\"password\",\"v\":\"123456\"},{\"n\":\"remember\",\"v\":true}]}"

        XCTAssertEqual("{\"r\":\"LOGGED-WITH-REMEMBER\"}", Client.callSync(request))
    }

    func testCallSyncAnswersEmptyFromAStringWhenTheMappingDefers() {
        let box = ResponderBox()

        MappingList.shared.add(name: "platform.deferred.answer", item: MappingItem { _, r in box.keep(r) })

        XCTAssertEqual("", Client.callSync("{\"f\":\"platform.deferred.answer\",\"p\":[]}"))

        box.responder?("deferred")
    }

    func testCallSyncAnswersEmptyFromAStringWhenNothingOwnsTheFunction() {
        XCTAssertEqual("", Client.callSync("{\"f\":\"not.found\",\"p\":[]}"))
    }

    func testCallSyncAnswersWhatTheNestedMappingProduced() {
        MappingList.shared.add(name: "platform.reverse.response", item: MappingItem(target: reverseResponse))

        let response: String? = Client.callSync(Request("sample.reverse"))

        XCTAssertEqual("response-is-ok", response)
    }
}

@MainActor
final class OrderBox {
    var order: [String] = []
}
