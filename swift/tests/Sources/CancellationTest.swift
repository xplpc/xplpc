import XCTest
import xplpc

final class CancellationTest: XPLPCTestCase {
    func testCancellingCallAsyncWhileTheMappingNeverAnswersAnswersNothing() async {
        // A task waiting on a mapping that never answers is resumed by the cancel, since nothing else would ever resume it.

        let received = expectation(description: "received")

        MappingList.shared.add(name: "platform.never.answers", item: MappingItem(target: { _, _ in
            received.fulfill()
        }))

        let task = Task { () -> String? in
            await Client.callAsync(Request("platform.never.answers"))
        }

        await fulfillment(of: [received], timeout: 5)

        task.cancel()

        await expectResumed(task) { XCTAssertNil($0) }
    }

    func testCancellingCallAsyncBeforeItStartsAnswersNothingAndSendsNothing() async {
        let invoked = Counter()

        MappingList.shared.add(name: "platform.counted", item: MappingItem(target: { _, r in
            invoked.increment()
            r("sent")
        }))

        let task = Task { () -> String? in
            withUnsafeCurrentTask { $0?.cancel() }
            return await Client.callAsync(Request("platform.counted"))
        }

        await expectResumed(task) { XCTAssertNil($0) }

        XCTAssertEqual(0, invoked.value)
    }

    func testCancellingCallAsyncWhileItIsBeingDispatchedAnswersNothing() async {
        // The mapping runs inside the dispatch on the caller's own task, so cancelling from there lands after the call was sent and before its handle is known.

        MappingList.shared.add(name: "platform.cancels.its.caller", item: MappingItem(target: { _, _ in
            withUnsafeCurrentTask { $0?.cancel() }
        }))

        let task = Task { () -> String? in
            await Client.callAsync(Request("platform.cancels.its.caller"))
        }

        await expectResumed(task) { XCTAssertNil($0) }
    }

    func testCallAsyncResolvesOnceWhenACancelRacesTheAnswer() async {
        // The answer and the cancel both try to take the call, so whichever wins resumes the caller and the other finds nothing, and a second resume would trap.

        MappingList.shared.add(name: "platform.answers.later", item: MappingItem(target: { _, r in
            DispatchQueue.global(qos: .userInitiated).async {
                r("later")
            }
        }))

        for index in 0 ..< 200 {
            let task = Task { () -> String? in
                await Client.callAsync(Request("platform.answers.later"))
            }

            if index % 2 == 0 {
                await Task.yield()
            }

            task.cancel()

            // One caller left waiting is enough to say what is wrong, so the loop stops there rather than waiting out every iteration.
            guard await expectResumed(task, { XCTAssertTrue($0 == nil || $0 == "later") }) else {
                return
            }
        }
    }

    func testCancelledCallAsyncIgnoresTheAnswerThatArrivesLater() async {
        let box = ResponderBox()
        let invocations = Counter()
        let first = expectation(description: "first")
        let second = expectation(description: "second")

        MappingList.shared.add(name: "platform.answers.when.told", item: MappingItem(target: { _, r in
            box.keep(r)
            invocations.increment()

            if invocations.value == 1 {
                first.fulfill()
                return
            }

            second.fulfill()
        }))

        let task = Task { () -> String? in
            await Client.callAsync(Request("platform.answers.when.told"))
        }

        await fulfillment(of: [first], timeout: 5)

        task.cancel()

        await expectResumed(task) { XCTAssertNil($0) }

        box.responder?("late")

        // The same mapping still answers a call made after the cancelled one, so the late answer reached nothing that is still waiting.
        let again = Task { () -> String? in
            await Client.callAsync(Request("platform.answers.when.told"))
        }

        await fulfillment(of: [second], timeout: 5)

        box.responder?("again")

        await expectResumed(again) { XCTAssertEqual("again", $0) }
    }

    /// Waits for the task to finish rather than awaiting it directly, so a caller that is never resumed fails the test instead of hanging the suite.
    @discardableResult
    private func expectResumed(_ task: Task<String?, Never>, _ check: @escaping @Sendable (String?) -> Void) async -> Bool {
        let resumed = XCTestExpectation(description: "resumed")

        Task {
            await check(task.value)
            resumed.fulfill()
        }

        guard await XCTWaiter().fulfillment(of: [resumed], timeout: 5) == .completed else {
            XCTFail("The caller was never resumed")
            return false
        }

        return true
    }
}
