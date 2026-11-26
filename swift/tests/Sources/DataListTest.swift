import XCTest
import xplpc

final class DataListTest: XPLPCTestCase {
    func testAMappingAnsweringTwiceCallsBackOnce() {
        // A call resolves exactly once, so the second answer finds nothing and the callback never hears it.

        let answered = Counter()
        var response: String?

        MappingList.shared.add(name: "platform.answers.twice", item: MappingItem { _, r in
            r("first")
            r("second")
        })

        Client.call(Request("platform.answers.twice")) { (result: String?) in
            response = result
            answered.increment()
        }

        XCTAssertEqual(1, answered.value)
        XCTAssertEqual("first", response)
    }

    func testAnswersRacingFromManyThreadsCallBackOnce() {
        let box = ResponderBox()
        let answered = Counter()

        MappingList.shared.add(name: "platform.answers.when.told", item: MappingItem { _, r in
            box.keep(r)
        })

        Client.call(Request("platform.answers.when.told")) { (_: String?) in
            answered.increment()
        }

        DispatchQueue.concurrentPerform(iterations: 16) { index in
            box.responder?(AnyCodable("answer-\(index)"))
        }

        XCTAssertEqual(1, answered.value)
    }

    func testClearingTheMappingListRetractsItsNames() {
        MappingList.shared.add(name: "sample.temporary", item: MappingItem { _, r in r("temporary") })

        XCTAssertTrue(MappingList.shared.has("sample.temporary"))
        XCTAssertEqual("temporary", Client.callSync(Request("sample.temporary")) as String?)

        MappingList.shared.clear()

        XCTAssertEqual(0, MappingList.shared.count())
        XCTAssertFalse(MappingList.shared.has("sample.temporary"))
        XCTAssertNil(Client.callSync(Request("sample.temporary")) as String?)
    }

    func testASwiftMappingDeclaredUnderACoreNameTakesItOverUntilCleared() {
        // The last layer to declare a name is the one that serves it, and retracting it gives the name back to the layer below.

        MappingList.shared.add(name: "sample.login", item: MappingItem(target: { _, r in
            r("FROM-SWIFT")
        }))

        let taken: String? = Client.callSync(loginRequest())

        MappingList.shared.clear()

        let restored: String? = Client.callSync(loginRequest())

        XCTAssertEqual("FROM-SWIFT", taken)
        XCTAssertEqual("LOGGED-WITH-REMEMBER", restored)
    }
}
