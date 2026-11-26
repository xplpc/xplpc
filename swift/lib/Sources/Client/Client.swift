import Foundation

public class Client {
    public static func call<T: Decodable>(_ request: Request, _ callback: ((_ result: T?) -> Void)? = nil) {
        ObjCLayerBridge.call(request.data) { response in
            callback?(decode(response, "call"))
        }
    }

    public static func call(_ requestData: String, callback: ((String) -> Void)?) {
        ObjCLayerBridge.call(requestData) { response in
            callback?(response)
        }
    }

    public static func callSync<T: Decodable>(_ request: Request) -> T? {
        guard let response = answerSynchronously(request.data) else {
            return nil
        }

        return decode(response, "callSync")
    }

    public static func callSync(_ requestData: String) -> String {
        answerSynchronously(requestData) ?? ""
    }

    /// A mapping that had not answered when the call returned is cancelled, so its later answer resolves nothing and leaves nothing behind.
    private static func answerSynchronously(_ data: String) -> String? {
        var answer: String?

        let call = ObjCLayerBridge.call(data) { response in
            answer = response
        }

        if call == 0 {
            return answer
        }

        ObjCLayerBridge.cancel(call)
        Log.e("[Client : callSync] The function did not answer synchronously")

        return nil
    }

    public static func callAsync<T: Decodable & Sendable>(_ request: Request) async -> T? {
        await decode(callAsync(request.data), "callAsync")
    }

    public static func callAsync(_ requestData: String) async -> String {
        let answer = AsyncAnswer()

        return await withTaskCancellationHandler {
            await withCheckedContinuation { continuation in
                answer.wait(continuation)

                // A task cancelled before this point is answered here, so the call is not sent at all.
                if Task.isCancelled {
                    answer.resolve("")
                    return
                }

                let call = ObjCLayerBridge.call(requestData) { response in
                    answer.resolve(response)
                }

                answer.dispatched(call)
            }
        } onCancel: {
            answer.cancel()
        }
    }

    private static func decode<T: Decodable>(_ response: String, _ entryPoint: String) -> T? {
        // The empty answer is what every failing path and a cancelled call answer with, and each reported it where it happened.
        if response.isEmpty {
            return nil
        }

        guard let config = XPLPC.shared.config else {
            Log.e("[Client : \(entryPoint)] XPLPC was not initialized")
            return nil
        }

        return config.serializer.decodeFunctionReturnValue(response)
    }

    /// The answer, the continuation and a cancel can arrive in any order and from any thread, and the caller is resumed exactly once.
    private final class AsyncAnswer: @unchecked Sendable {
        private let lock = NSLock()
        private var continuation: CheckedContinuation<String, Never>?
        private var call: UInt64 = 0
        private var cancelled = false

        /// Takes the continuation before anything can answer, since the call is only sent once it is held.
        func wait(_ waiting: CheckedContinuation<String, Never>) {
            lock.lock()
            defer { lock.unlock() }

            continuation = waiting
        }

        func resolve(_ data: String) {
            lock.lock()
            let waiting = continuation
            continuation = nil
            lock.unlock()

            waiting?.resume(returning: data)
        }

        func dispatched(_ handle: UInt64) {
            lock.lock()
            call = handle
            let cancelRequested = cancelled
            lock.unlock()

            if cancelRequested {
                withdraw(handle)
            }
        }

        func cancel() {
            lock.lock()
            cancelled = true
            let handle = call
            lock.unlock()

            withdraw(handle)
        }

        /// Taking the call back is what decides between the answer and the cancel, since whichever loses resolves nothing.
        private func withdraw(_ handle: UInt64) {
            if handle != 0, ObjCLayerBridge.cancel(handle) {
                resolve("")
            }
        }
    }
}
