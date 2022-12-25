import { XRequest } from "../message/request";
import { XPLPCModule } from "../module/xplpc";

export class XRemoteClient {
    static wasmModule: XPLPCModule;

    static call<T>(request: XRequest): T | null {
        if (!XRemoteClient.wasmModule) {
            console.error("The WASM module is not initialized");
            return null;
        }

        try {
            return JSON.parse(XRemoteClient.wasmModule.ProxyClient.call(request.data())).r;
        } catch (e: unknown) {
            let error = "unknown";

            if (typeof e === "string") {
                error = e.toUpperCase()
            } else if (e instanceof Error) {
                error = e.message
            }

            console.error("[XRemoteClient : call] Error when call function: " + error);

            return null;
        }
    }

    static callAsync<T>(request: XRequest): Promise<T | null> {
        return new Promise<T | null>((resolve) => {
            (async function () {
                resolve(XRemoteClient.call<T>(request));
            })();
        });
    }
}
