import Foundation
import XCTest
import xplpc

/// Counts what happened from any thread, which is what lets a test assert a callback ran exactly once.
final class Counter: @unchecked Sendable {
    private let lock = NSLock()
    private var count = 0

    var value: Int {
        lock.lock()
        defer { lock.unlock() }

        return count
    }

    func increment() {
        lock.lock()
        defer { lock.unlock() }

        count += 1
    }
}

/// Holds the responder a deferred mapping received, so the test decides when and from where it answers.
final class ResponderBox: @unchecked Sendable {
    private let lock = NSLock()
    private var stored: Response?

    var responder: Response? {
        lock.lock()
        defer { lock.unlock() }

        return stored
    }

    func keep(_ responder: @escaping Response) {
        lock.lock()
        defer { lock.unlock() }

        stored = responder
    }
}

func loginRequest() -> Request {
    Request(
        "sample.login",
        Param("username", "paulo"),
        Param("password", "123456"),
        Param("remember", true)
    )
}

/// Counts how many times each value arrived, from any thread.
final class Tally: @unchecked Sendable {
    private let lock = NSLock()
    private var counts = [Int: Int]()

    var distinct: Int {
        lock.lock()
        defer { lock.unlock() }

        return counts.count
    }

    func record(_ value: Int) {
        lock.lock()
        defer { lock.unlock() }

        counts[value, default: 0] += 1
    }

    func count(for value: Int) -> Int {
        lock.lock()
        defer { lock.unlock() }

        return counts[value] ?? 0
    }
}

/// Every suite inherits this, so each test is held to leaving no call waiting anywhere in the process.
class XPLPCTestCase: XCTestCase {
    override func tearDown() {
        XCTAssertEqual(0, pendingCalls())

        super.tearDown()
    }

    /// Waits for the callback, since one that never runs would leave every check unexecuted and the test green, and fails a callback that runs twice.
    func verifyCall<T: Decodable>(_ request: Request, _ block: @escaping (T?) -> Void) {
        let answered = expectation(description: "answered")
        answered.assertForOverFulfill = true

        Client.call(request) { (response: T?) in
            block(response)
            answered.fulfill()
        }

        wait(for: [answered], timeout: 5)
    }

    func verifyCall(_ requestData: String, _ block: @escaping (String) -> Void) {
        let answered = expectation(description: "answered")
        answered.assertForOverFulfill = true

        Client.call(requestData) { response in
            block(response)
            answered.fulfill()
        }

        wait(for: [answered], timeout: 5)
    }

    /// Makes the call from a background thread, and the test still waits for the answer on its own.
    func verifyCallFromBackground<T: Decodable & Sendable>(_ request: Request, _ block: @escaping @Sendable (T?) -> Void) {
        let answered = expectation(description: "answered")
        answered.assertForOverFulfill = true

        DispatchQueue.global(qos: .userInitiated).async {
            Client.call(request) { (response: T?) in
                block(response)
                answered.fulfill()
            }
        }

        wait(for: [answered], timeout: 5)
    }
}

func pendingCalls() -> UInt {
    ObjCLayerBridge.pending()
}

/// Counts the lines the native library reported from one place while the body ran, since it writes them to standard output on every apple platform and the suite has no other way to read them.
func nativeErrors(from place: String, during body: () -> Void) -> Int {
    var ends: [Int32] = [0, 0]

    guard pipe(&ends) == 0 else {
        XCTFail("The pipe that captures native lines could not be opened")
        return -1
    }

    fflush(nil)

    let saved = dup(STDOUT_FILENO)
    dup2(ends[1], STDOUT_FILENO)
    close(ends[1])

    body()

    fflush(nil)
    dup2(saved, STDOUT_FILENO)
    close(saved)

    let captured = FileHandle(fileDescriptor: ends[0], closeOnDealloc: true).readDataToEndOfFile()

    // What was captured is written back, so a failing run still shows every line the native side reported.
    FileHandle.standardOutput.write(captured)

    return (String(bytes: captured, encoding: .utf8) ?? "")
        .split(separator: "\n")
        .filter { $0.contains(place) }
        .count
}
