import { XPLPC } from "../core/xplpc";
import { XCallbackList } from "../data/callback-list";
import { XRequest } from "../message/request";
import { ExceptionMessage } from "../util/exception-message";
import { Log } from "../util/log";
import { UniqueID } from "../util/unique-id";

export class XClient {
    static call<T>(request: XRequest): Promise<T | undefined>;

    static call(data: string): Promise<string>;

    static call<T>(request: XRequest | string): Promise<T | undefined> | Promise<string> {
        return new Promise<T | undefined>((resolve) => {
            if (!XPLPC.shared().initialized) {
                Log.e("[XClient : call] The WASM module is not initialized");
                return resolve(undefined);
            }

            try {
                UniqueID.generate().then((key: string) => {
                    if (typeof request === 'string') {
                        XCallbackList.shared().add(key, (response: string) => {
                            return resolve(response as T | undefined);
                        });

                        XPLPC.shared().module.Client.call(request, (response: string) => {
                            XCallbackList.shared().execute(key, response);
                        });
                    } else {
                        XCallbackList.shared().add(key, (response: string) => {
                            return resolve(XPLPC.shared().config.serializer.decodeFunctionReturnValue<T>(response));
                        });

                        XPLPC.shared().module.Client.call(request.data(), (response: string) => {
                            XCallbackList.shared().execute(key, response);
                        });
                    }
                });
            } catch (e: unknown) {
                Log.e("[XClient : call] Error: " + ExceptionMessage.get(e));
                return resolve(undefined);
            }
        });
    }
}
