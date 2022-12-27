import { XPLPCModule } from "../module/xplpc";

export interface IXNativeProxyClient {
    call<T>(data: string): T;
    callAsync<T>(data: string): Promise<T>;
}

export class XProxyClient {
    static wasmModule: XPLPCModule;

    static call(data: string): string {
        if (!XProxyClient.wasmModule) {
            console.error("[XProxyClient : call] The WASM module is not initialized");
            return "";
        }

        // TODO: XPLPC - NEED IMPLEMENTS
        console.log("[XProxyClient : call] Received data: ", data);
        return "";
    }

    static callAsync(data: string): Promise<string> {
        return new Promise<string>((resolve) => {
            (async function () {
                resolve(XProxyClient.call(data));
            })();
        });
    }
}
