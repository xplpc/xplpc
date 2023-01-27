public class RemoteClient {
    internal class RemoteClientReturn<T: Decodable>: Decodable {
        var r: T?
    }

    public static func call<T: Decodable>(_ request: Request, _ callback: ((_ result: T?) -> Void)? = nil) {
        let key = UniqueID.generate()

        CallbackList.shared.add(key: key) { (response: String) in
            guard let data: RemoteClientReturn<T> = XPLPC.shared.config.serializer.decodeFunctionReturnValue(response) else {
                Log.e("[RemoteClient : call] Error when decode data")
                callback?(nil)
                return
            }

            callback?(data.r)
        }

        PlatformProxy.shared.nativeProxyCall(key, request.data)
    }
}
