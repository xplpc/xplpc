import Foundation

public class Client {
    class ClientReturn<T: Decodable>: Decodable {
        var r: T?
    }

    /// A mapping is free to answer from a thread of its own after callSync has returned, so what it writes into outlives that frame and is guarded.
    private final class SyncAnswer {
        private let lock = NSLock()
        private var value: String?

        func set(_ data: String) {
            lock.lock()
            defer { lock.unlock() }

            value = data
        }

        func get() -> String? {
            lock.lock()
            defer { lock.unlock() }

            return value
        }
    }

    public static func call<T: Decodable>(_ request: Request, _ callback: ((_ result: T?) -> Void)? = nil) {
        let key = UniqueID.generate()

        CallbackList.shared.add(key: key) { (response: String) in
            guard let config = XPLPC.shared.config,
                  let data: ClientReturn<T> = config.serializer.decodeFunctionReturnValue(response)
            else {
                Log.e("[Client : call] Error when decode data")
                callback?(nil)
                return
            }

            callback?(data.r)
        }

        PlatformProxy.shared.callNativeProxy(key, request.data)
    }

    public static func call(_ requestData: String, callback: ((String) -> Void)?) {
        let key = UniqueID.generate()

        CallbackList.shared.add(key: key) { (response: String) in
            callback?(response)
        }

        PlatformProxy.shared.callNativeProxy(key, requestData)
    }

    public static func callSync<T: Decodable>(_ request: Request) -> T? {
        guard let response = answerSynchronously(request.data) else {
            return nil
        }

        guard let config = XPLPC.shared.config,
              let data: ClientReturn<T> = config.serializer.decodeFunctionReturnValue(response)
        else {
            Log.e("[Client : callSync] Error when decode data")
            return nil
        }

        return data.r
    }

    public static func callSync(_ requestData: String) -> String {
        answerSynchronously(requestData) ?? ""
    }

    private static func answerSynchronously(_ data: String) -> String? {
        let answer = SyncAnswer()
        let key = UniqueID.generate()

        CallbackList.shared.add(key: key) { (response: String) in
            answer.set(response)
        }

        PlatformProxy.shared.callNativeProxy(key, data)

        // Taking the key back is what decides the two cases, since a mapping that answered inline has already taken it and one that deferred never will.
        CallbackList.shared.remove(key: key)

        let response = answer.get()

        if response == nil {
            Log.e("[Client : callSync] The function did not answer synchronously")
        }

        return response
    }

    @available(iOS 13.0, macOS 10.15, macCatalyst 13.0, watchOS 6.0, tvOS 13.0, visionOS 1.0, *)
    public static func callAsync<T: Decodable>(_ request: Request) async -> T? {
        await withCheckedContinuation { continuation in
            var resumed = false

            call(request) { (result: T?) in
                guard !resumed else {
                    Log.e("[Client : callAsync] Response was resolved more than once")
                    return
                }

                resumed = true
                continuation.resume(returning: result)
            }
        }
    }

    @available(iOS 13.0, macOS 10.15, macCatalyst 13.0, watchOS 6.0, tvOS 13.0, visionOS 1.0, *)
    public static func callAsync(_ requestData: String) async -> String {
        await withCheckedContinuation { continuation in
            var resumed = false

            call(requestData) { response in
                guard !resumed else {
                    Log.e("[Client : callAsync] Response was resolved more than once")
                    return
                }

                resumed = true
                continuation.resume(returning: response)
            }
        }
    }
}
