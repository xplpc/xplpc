import XCTest
import xplpc

final class StressTest: XCTestCase {
    private let threadCount = 16
    private let iterationCount = 400

    override func setUpWithError() throws {
        // Each test starts from a clean registry, so the order they run in cannot change the result.

        CallbackList.shared.clear()
        MappingList.shared.clear()
    }

    override func tearDownWithError() throws {
        CallbackList.shared.clear()
        MappingList.shared.clear()
    }

    func testCallbackListSurvivesContention() {
        let executed = NSCountedSet()
        let guardLock = NSLock()

        DispatchQueue.concurrentPerform(iterations: threadCount) { index in
            for i in 0 ..< iterationCount {
                let key = "stress-\(i % 32)"

                CallbackList.shared.add(key: key) { _ in
                    guardLock.lock()
                    executed.add("call")
                    guardLock.unlock()
                }

                CallbackList.shared.execute(key: key, data: "data")
                _ = CallbackList.shared.count()

                if index % 2 == 0 {
                    CallbackList.shared.remove(key: key)
                }
            }
        }

        XCTAssertGreaterThan(executed.count(for: "call"), 0)
    }

    func testMappingListSurvivesContention() {
        DispatchQueue.concurrentPerform(iterations: threadCount) { index in
            for i in 0 ..< iterationCount {
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

    func testCallbackListNeverBlocksOnAMissingKey() {
        // A lock released on only one path would leave every later call waiting forever.

        for i in 0 ..< 1000 {
            CallbackList.shared.execute(key: "missing-\(i)", data: "")
        }

        CallbackList.shared.add(key: "after-misses") { _ in }

        XCTAssertEqual(1, CallbackList.shared.count())
    }

    func testClientAnswersEveryCallFromEveryThread() {
        let answered = NSCountedSet()
        let guardLock = NSLock()

        MappingList.shared.add(name: "stress.echo", item: MappingItem { m, r in
            r(AnyCodable(m.get("value") as String?))
        })

        DispatchQueue.concurrentPerform(iterations: threadCount) { _ in
            for _ in 0 ..< 64 {
                let request = Request("stress.echo", Param("value", "stress"))

                Client.call(request) { (response: String?) in
                    if response == "stress" {
                        guardLock.lock()
                        answered.add("call")
                        guardLock.unlock()
                    }
                }
            }
        }

        XCTAssertEqual(threadCount * 64, answered.count(for: "call"))
        XCTAssertEqual(0, CallbackList.shared.count())
    }

    func testCallSyncAgainstAMappingRacingToAnswer() {
        // The responder is handed to a worker that is already running, so the answer lands inside the window between the call being dispatched and its key being taken back.
        // What it writes into has to survive that frame and be safe to read while it is being written.

        let queueLock = NSLock()
        var queue = [Response]()
        let running = NSLock()
        var stopped = false

        let worker = Thread {
            while true {
                running.lock()
                let done = stopped
                running.unlock()

                if done {
                    return
                }

                queueLock.lock()
                let responder = queue.popLast()
                queueLock.unlock()

                responder?("racing")
            }
        }

        worker.start()

        MappingList.shared.add(name: "test.sync.racing", item: MappingItem { _, r in
            queueLock.lock()
            queue.append(r)
            queueLock.unlock()
        })

        DispatchQueue.concurrentPerform(iterations: threadCount) { _ in
            for _ in 0 ..< 64 {
                let response: String? = Client.callSync(Request("test.sync.racing"))

                XCTAssertTrue(response == nil || response == "racing")
            }
        }

        running.lock()
        stopped = true
        running.unlock()

        // How many callers read the answer in time is decided by the race, and what has to hold whichever side wins the key is that nothing is left registered.
        XCTAssertEqual(0, CallbackList.shared.count())
    }
}
