import { XPLPC } from "../core/xplpc";
import { XCallbackList } from "../data/callback-list";
import { XRequest } from "../message/request";
import { ExceptionMessage } from "../util/exception-message";
import { Log } from "../util/log";
import { UniqueID } from "../util/unique-id";

export class XRemoteClient {
    static call<T>(request: XRequest): Promise<T | undefined> {
        return new Promise<T | undefined>((resolve) => {
            if (!XPLPC.shared().initialized) {
                Log.e("[XRemoteClient : call] The WASM module is not initialized");
                return resolve(undefined);
            }

            try {
                UniqueID.generate().then((key: string) => {
                    XCallbackList.shared().add(key, (response: string) => {
                        return resolve(XPLPC.shared().config.serializer.decodeFunctionReturnValue<T>(response));
                    });

                    XPLPC.shared().module.ProxyClient.callFromJavascript(request.data(), (response: string) => {
                        XCallbackList.shared().execute(key, response);
                    });
                });
            } catch (e: unknown) {
                Log.e("[XRemoteClient : call] Error: " + ExceptionMessage.get(e));
                return resolve(undefined);
            }
        });
    }
}
