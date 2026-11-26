export interface HostLayer {
    invoke(call: bigint, data: string): void;
}

interface NativeJavascriptBridge {
    attach(host: HostLayer): void;
    declare(name: string): void;
    retract(name: string): void;
    call(data: string, callback: (response: string) => void): bigint;
    cancel(call: bigint): boolean;
    answer(call: bigint, data: string): void;
    registerBuffer(address: number, size: number): bigint;
    releaseBuffer(buffer: bigint): boolean;
    readBuffer(buffer: bigint): Uint8Array<ArrayBuffer> | null;
    pendingCount(): number;
}

export interface WasmModule {
    JavascriptBridge: NativeJavascriptBridge;

    // The emscripten runtime exports are enabled through EXPORTED_FUNCTIONS and EXPORTED_RUNTIME_METHODS.
    HEAPU8: Uint8Array<ArrayBuffer>;
    _malloc(size: number): number;
    _free(ptr: number): void;
}
