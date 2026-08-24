import XCTest
import xplpc

final class DataListTest: XCTestCase {
    override func tearDownWithError() throws {
        CallbackList.shared.clear()
    }

    func testCallbackListExecutesOnlyOnce() {
        var calls = 0

        CallbackList.shared.add(key: "callback-once") { _ in
            calls += 1
        }

        CallbackList.shared.execute(key: "callback-once", data: "")
        CallbackList.shared.execute(key: "callback-once", data: "")

        XCTAssertEqual(1, calls)
    }

    func testCallbackListExecutesUnknownKeyWithoutBlocking() {
        // An unknown key used to leave the lock held forever, blocking every later call.

        CallbackList.shared.execute(key: "callback-unknown", data: "")

        CallbackList.shared.add(key: "callback-after-unknown") { _ in }

        XCTAssertEqual(1, CallbackList.shared.count())
    }

    func testCallbackListRemovesPendingEntry() {
        var calls = 0

        CallbackList.shared.add(key: "callback-removed") { _ in
            calls += 1
        }

        CallbackList.shared.remove(key: "callback-removed")
        CallbackList.shared.execute(key: "callback-removed", data: "")

        XCTAssertEqual(0, calls)
        XCTAssertEqual(0, CallbackList.shared.count())
    }

    func testCallbackListIsTakenByASingleThread() {
        let counter = NSCountedSet()
        let lock = NSLock()

        CallbackList.shared.add(key: "callback-concurrent") { _ in
            lock.lock()
            counter.add("call")
            lock.unlock()
        }

        DispatchQueue.concurrentPerform(iterations: 16) { _ in
            CallbackList.shared.execute(key: "callback-concurrent", data: "")
        }

        XCTAssertEqual(1, counter.count(for: "call"))
    }

    func testMappingListSurvivesClear() {
        MappingList.shared.add(name: "sample.temporary", item: MappingItem { _, r in r(nil) })

        XCTAssertTrue(MappingList.shared.has("sample.temporary"))

        MappingList.shared.clear()

        XCTAssertEqual(0, MappingList.shared.count())
        XCTAssertFalse(MappingList.shared.has("sample.temporary"))
    }

    func testByteArrayHelperReturnsEmptyForInvalidPointer() {
        XCTAssertTrue(ByteArrayHelper.createFromPtr(ptr: 0, size: 8).isEmpty)
        XCTAssertTrue(ByteArrayHelper.createFromDataView(DataView(ptr: 0, size: 0)).isEmpty)
    }
}
