import XCTest
import xplpc

final class ClientTest: XCTestCase {
    override func tearDown() {
        // A leaked callback is the defect this project has found most often, so every test is held to leaving none.
        XCTAssertEqual(CallbackList.shared.count(), 0)

        super.tearDown()
    }

    func deferredReverseResponse(message _: Message, r: @escaping Response) {
        DispatchQueue.global(qos: .userInitiated).asyncAfter(deadline: .now() + 0.05) {
            r("ok")
        }
    }

    func deferredAnswer(message _: Message, r: @escaping Response) {
        DispatchQueue.global(qos: .userInitiated).asyncAfter(deadline: .now() + 0.05) {
            r("deferred")
        }
    }

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

    func verifyCall<T: Decodable>(_ request: Request, _ block: @escaping (T?) -> Void) {
        // A callback that never runs would leave every check unexecuted and the test green, so the test waits for it.

        let answered = expectation(description: "answered")

        Client.call(request) { (response: T?) in
            block(response)
            answered.fulfill()
        }

        wait(for: [answered], timeout: 5)
    }

    func verifyCall(_ requestData: String, _ block: @escaping (String) -> Void) {
        let answered = expectation(description: "answered")

        Client.call(requestData) { response in
            block(response)
            answered.fulfill()
        }

        wait(for: [answered], timeout: 5)
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

    #if compiler(>=5.5) && canImport(_Concurrency)
        @available(iOS 13.0, macOS 10.15, macCatalyst 13.0, watchOS 6.0, tvOS 13.0, visionOS 1.0, *)
        @MainActor
        func testCallAsyncGivesTheSchedulerATurnBeforeAnswering() async {
            // A call site has to behave the same whether the mapping is quick or slow, so the scheduler is reached even when the answer is already there.

            let box = OrderBox()

            Task { @MainActor in box.order.append("scheduler") }

            let _: String? = await Client.callAsync(Request("sample.version"))
            box.order.append("after await")

            XCTAssertEqual(["scheduler", "after await"], box.order)
        }

        @available(iOS 13.0, macOS 10.15, macCatalyst 13.0, watchOS 6.0, tvOS 13.0, visionOS 1.0, *)
        func testBatteryLevelCallAsync() async {
            MappingList.shared.add(name: "platform.battery.level", item: MappingItem(target: batteryLevel))

            let request = Request("platform.battery.level", Param("suffix", "%"))

            if let response: String? = await Client.callAsync(request) {
                XCTAssertEqual("100%", response)
            }
        }
    #endif

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

    func testGrayscaleImageWithDataView() {
        var data: [UInt8] = [
            255, 0, 0, 255,
            0, 255, 0, 255,
            0, 0, 255, 255,
            0, 0, 0, 0,
        ]

        var processed: [UInt8] = []
        let answered = expectation(description: "answered")

        DataView.withByteArray(&data) { dataView in
            let request = Request(
                "sample.image.grayscale.dataview",
                Param("dataView", dataView)
            )

            Client.call(request) { (response: String?) in
                XCTAssertEqual("OK", response)
                processed = ByteArrayHelper.createFromDataView(dataView)
                answered.fulfill()
            }

            wait(for: [answered], timeout: 5)
        }

        XCTAssertEqual(16, processed.count)
        XCTAssertEqual(processed[0], 85)
        XCTAssertEqual(processed[4], 85)
        XCTAssertEqual(processed[8], 85)
        XCTAssertEqual(processed[12], 0)
    }

    func testDataView() {
        let request = Request("sample.dataview")

        verifyCall(request) { (r: DataView?) in
            XCTAssertNotNil(r)

            guard let dataView = r else {
                return
            }

            let originalData = ByteArrayHelper.createFromDataView(dataView)

            XCTAssertEqual(16, dataView.size)
            XCTAssertEqual(originalData[0], 255)
            XCTAssertEqual(originalData[5], 255)
            XCTAssertEqual(originalData[10], 255)
            XCTAssertEqual(originalData[12], 0)

            let request2 = Request(
                "sample.image.grayscale.dataview",
                Param("dataView", dataView)
            )

            self.verifyCall(request2) { (response: String?) in
                XCTAssertEqual("OK", response)

                let processedData = ByteArrayHelper.createFromDataView(dataView)

                XCTAssertEqual(16, processedData.count)
                XCTAssertEqual(processedData[0], 85)
                XCTAssertEqual(processedData[4], 85)
                XCTAssertEqual(processedData[8], 85)
                XCTAssertEqual(processedData[12], 0)

                XCTAssertEqual(16, dataView.size)
                XCTAssertEqual(originalData[0], 255)
                XCTAssertEqual(originalData[5], 255)
                XCTAssertEqual(originalData[10], 255)
                XCTAssertEqual(originalData[12], 0)
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

    #if compiler(>=5.5) && canImport(_Concurrency)
        @available(iOS 13.0, macOS 10.15, macCatalyst 13.0, watchOS 6.0, tvOS 13.0, visionOS 1.0, *)
        func testBatteryLevelCallAsyncFromString() async {
            MappingList.shared.add(name: "platform.battery.level", item: MappingItem(target: batteryLevel))

            let request = Request("platform.battery.level", Param("suffix", "%"))
            let response: String = await Client.callAsync(request.data)

            XCTAssertEqual("{\"r\":\"100%\"}", response)
        }
    #endif

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
        // A buffer far past what a small integer holds has to cross without being truncated anywhere on the way.

        let size = 4 * 1024 * 1024
        var bytes = [UInt8](repeating: 100, count: size)

        DataView.withByteArray(&bytes) { dataView in
            let request = Request("sample.image.grayscale.dataview", Param("dataView", dataView))

            verifyCall(request) { (response: String?) in
                XCTAssertEqual("OK", response)
                XCTAssertEqual(UInt(size), dataView.size)
            }
        }

        XCTAssertEqual(100, bytes[0])
        XCTAssertEqual(100, bytes[size - 1])
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
        MappingList.shared.add(name: "platform.reverse.response", item: MappingItem(target: deferredReverseResponse))

        let answered = expectation(description: "nested")
        var response: String?

        Client.call(Request("sample.reverse")) { (result: String?) in
            response = result
            answered.fulfill()
        }

        wait(for: [answered], timeout: 5)

        XCTAssertEqual("response-is-ok", response)
    }

    func testHostMappingAnswersAfterTheCallReturned() {
        MappingList.shared.add(name: "platform.deferred.answer", item: MappingItem(target: deferredAnswer))

        let answered = expectation(description: "deferred")
        var response: String?

        Client.call(Request("platform.deferred.answer")) { (result: String?) in
            response = result
            answered.fulfill()
        }

        XCTAssertNil(response)

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

    func testUnknownFunctionAnswersEmptyAndLeavesNothingRegistered() {
        // Nothing owns this name, so the caller is answered with the empty value and the registration is not left behind.

        let before = CallbackList.shared.count()

        verifyCall(Request("not.found")) { (response: String?) in
            XCTAssertNil(response)
        }

        XCTAssertEqual(before, CallbackList.shared.count())
    }

    func testCallSyncAnswersTheValueFromAnInlineMapping() {
        // A mapping that answers before it returns has already resolved the key, so the value is there to be read.

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
        // A mapping that answers later cannot be read synchronously, and the registration it would have resolved is dropped rather than left behind.

        MappingList.shared.add(name: "platform.deferred.answer", item: MappingItem(target: deferredAnswer))

        let before = CallbackList.shared.count()
        let response: String? = Client.callSync(Request("platform.deferred.answer"))

        XCTAssertNil(response)
        XCTAssertEqual(before, CallbackList.shared.count())

        // The mapping answers into a registration that is already gone, and the suite waits for it rather than leaving it running past the process.
        Thread.sleep(forTimeInterval: 0.2)

        XCTAssertEqual(before, CallbackList.shared.count())
    }

    func testCallSyncAnswersEmptyWhenNothingOwnsTheFunction() {
        let before = CallbackList.shared.count()
        let response: String? = Client.callSync(Request("not.found"))

        XCTAssertNil(response)
        XCTAssertEqual(before, CallbackList.shared.count())
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
        MappingList.shared.add(name: "platform.deferred.answer", item: MappingItem(target: deferredAnswer))

        let before = CallbackList.shared.count()

        XCTAssertEqual("", Client.callSync("{\"f\":\"platform.deferred.answer\",\"p\":[]}"))
        XCTAssertEqual(before, CallbackList.shared.count())

        Thread.sleep(forTimeInterval: 0.2)
    }

    func testCallSyncAnswersEmptyFromAStringWhenNothingOwnsTheFunction() {
        let before = CallbackList.shared.count()

        XCTAssertEqual("", Client.callSync("{\"f\":\"not.found\",\"p\":[]}"))
        XCTAssertEqual(before, CallbackList.shared.count())
    }

    func testCallSyncAnswersWhatTheNestedMappingProduced() {
        MappingList.shared.add(name: "platform.reverse.response", item: MappingItem(target: reverseResponse))

        let response: String? = Client.callSync(Request("sample.reverse"))

        XCTAssertEqual("response-is-ok", response)
    }
}

@available(iOS 13.0, macOS 10.15, macCatalyst 13.0, watchOS 6.0, tvOS 13.0, visionOS 1.0, *)
@MainActor
final class OrderBox {
    var order: [String] = []
}
