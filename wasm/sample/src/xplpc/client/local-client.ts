import { XWasmModule } from "../interface/wasm-module";
import { XRequest } from "../message/request";

export class XLocalClient {
    static wasmModule: XWasmModule;

    static call<T>(request: XRequest): T | null {
        if (!XLocalClient.wasmModule) {
            console.error("The WASM module is not initialized");
            return null;
        }

        try {
            return JSON.parse(XLocalClient.wasmModule.ProxyClient.call(request.data())).r;
        } catch (e: unknown) {
            let error = "unknown";

            if (typeof e === "string") {
                error = e.toUpperCase()
            } else if (e instanceof Error) {
                error = e.message
            }

            console.error("[XLocalClient : call] Error when call function: " + error)

            return null;
        }
    }

    static callAsync<T>(request: XRequest): Promise<T | null> {
        return new Promise<T | null>((resolve) => {
            (async function () {
                resolve(XLocalClient.call<T>(request));
            })();
        });
    }
}
