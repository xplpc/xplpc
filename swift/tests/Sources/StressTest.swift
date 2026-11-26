import XCTest
import xplpc

final class StressTest: XPLPCTestCase {
    private let threadCount = 16

    override func setUpWithError() throws {
        // Each test starts from a clean registry, so the order they run in cannot change the result.

        MappingList.shared.clear()
    }

    override func tearDownWithError() throws {
        MappingList.shared.clear()
    }

    func testDeferredAnswersFromEveryThreadEachArriveOnce() {
        // Every call is answered later from a queue of its own, so the router resolves them while new ones are still being made from other threads.

        let total = threadCount * 64
        let answered = Tally()
        let group = DispatchGroup()

        MappingList.shared.add(name: "stress.deferred.echo", item: MappingItem { m, r in
            let value: Int? = m.get("value")

            DispatchQueue.global(qos: .userInitiated).async {
                r(AnyCodable(value))
            }
        })

        for _ in 0 ..< total {
            group.enter()
        }

        DispatchQueue.concurrentPerform(iterations: threadCount) { thread in
            for i in 0 ..< 64 {
                let index = thread * 64 + i

                Client.call(Request("stress.deferred.echo", Param("value", index))) { (response: Int?) in
                    answered.record(response ?? -1)
                    group.leave()
                }
            }
        }

        XCTAssertEqual(.success, group.wait(timeout: .now() + 10))
        XCTAssertEqual(total, answered.distinct)

        for index in 0 ..< total {
            XCTAssertEqual(1, answered.count(for: index))
        }
    }

    func testMappingListSurvivesContention() {
        let iterations = 400

        DispatchQueue.concurrentPerform(iterations: threadCount) { index in
            for i in 0 ..< iterations {
                let name = "stress.mapping.\(i % 16)"

                MappingList.shared.add(name: name, item: MappingItem { _, r in r(nil) })
                _ = MappingList.shared.find(name)
                _ = MappingList.shared.has(name)
                _ = MappingList.shared.count()

                if index == 0, i % 64 == 0 {
                    MappingList.shared.clear()
                }
            }
        }
    }

    func testAClearRacingAnAddNeverLeavesANameRoutedToNothing() {
        // The core owns this name as well, so swift either serves it or gives it back, and an empty answer means the router still routes it to a mapping swift no longer holds.

        for _ in 0 ..< 2000 {
            let start = StartLine()
            let group = DispatchGroup()

            DispatchQueue.global(qos: .userInitiated).async(group: group) {
                start.wait()
                MappingList.shared.add(name: "sample.login", item: MappingItem { _, r in r("FROM-SWIFT") })
            }

            DispatchQueue.global(qos: .userInitiated).async(group: group) {
                start.wait()
                MappingList.shared.clear()
            }

            start.open()
            group.wait()

            let expected = MappingList.shared.has("sample.login") ? "FROM-SWIFT" : "LOGGED-WITH-REMEMBER"
            let response: String? = Client.callSync(loginRequest())

            guard response == expected else {
                XCTFail("Function \"sample.login\" answered \(response ?? "nothing") while swift \(expected == "FROM-SWIFT" ? "holds" : "does not hold") its mapping")
                return
            }
        }
    }

    func testUnknownFunctionsNeverBlockALaterCall() {
        // A lock released on only one path would leave every later call waiting forever.

        for _ in 0 ..< 200 {
            let response: String? = Client.callSync(Request("stress.missing"))

            XCTAssertNil(response)
        }

        let response: String? = Client.callSync(loginRequest())

        XCTAssertEqual("LOGGED-WITH-REMEMBER", response)
    }

    func testClientAnswersEveryCallFromEveryThread() {
        // Every call carries its own index back, so a call answered twice and one never answered cannot cancel each other out in the count.

        let total = threadCount * 64
        let answered = Tally()

        MappingList.shared.add(name: "stress.echo", item: MappingItem { m, r in
            r(AnyCodable(m.get("value") as Int?))
        })

        DispatchQueue.concurrentPerform(iterations: threadCount) { thread in
            for i in 0 ..< 64 {
                let request = Request("stress.echo", Param("value", thread * 64 + i))

                Client.call(request) { (response: Int?) in
                    answered.record(response ?? -1)
                }
            }
        }

        XCTAssertEqual(total, answered.distinct)

        for index in 0 ..< total {
            XCTAssertEqual(1, answered.count(for: index))
        }
    }

    func testCallSyncAgainstAMappingRacingToAnswer() {
        // The responder is handed to a worker that is already running, so the answer lands inside the window between the call being dispatched and being cancelled.

        let responders = ResponderQueue()

        let worker = Thread {
            while !responders.isStopped {
                responders.pop()?("racing")
            }
        }

        worker.start()

        MappingList.shared.add(name: "test.sync.racing", item: MappingItem { _, r in
            responders.push(r)
        })

        DispatchQueue.concurrentPerform(iterations: threadCount) { _ in
            for _ in 0 ..< 64 {
                let response: String? = Client.callSync(Request("test.sync.racing"))

                XCTAssertTrue(response == nil || response == "racing")
            }
        }

        // How many callers read the answer in time is decided by the race, and what has to hold whichever side wins the call is that each caller got one answer and the thread sanitizer saw nothing.
        responders.stop()
    }
}

/// Holds every thread that waits on it until it opens, so two operations start as close together as the scheduler allows.
private final class StartLine: @unchecked Sendable {
    private let lock = NSLock()
    private var opened = false

    func open() {
        lock.lock()
        defer { lock.unlock() }

        opened = true
    }

    func wait() {
        while !isOpen {}
    }

    private var isOpen: Bool {
        lock.lock()
        defer { lock.unlock() }

        return opened
    }
}

/// Both fields are only touched under the lock, which is what the unchecked conformance asserts.
private final class ResponderQueue: @unchecked Sendable {
    private let lock = NSLock()
    private var responders = [Response]()
    private var stopped = false

    var isStopped: Bool {
        lock.lock()
        defer { lock.unlock() }

        return stopped
    }

    func push(_ responder: @escaping Response) {
        lock.lock()
        defer { lock.unlock() }

        responders.append(responder)
    }

    func pop() -> Response? {
        lock.lock()
        defer { lock.unlock() }

        return responders.popLast()
    }

    func stop() {
        lock.lock()
        defer { lock.unlock() }

        stopped = true
    }
}
