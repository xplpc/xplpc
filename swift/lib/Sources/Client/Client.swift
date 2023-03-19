public class Client {
    internal class ClientReturn<T: Decodable>: Decodable {
        var r: T?
    }

    public static func call<T: Decodable>(_ request: Request, _ callback: ((_ result: T?) -> Void)? = nil) {
        let key = UniqueID.generate()

        CallbackList.shared.add(key: key) { (response: String) in
            guard let data: ClientReturn<T> = XPLPC.shared.config.serializer.decodeFunctionReturnValue(response) else {
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
}
