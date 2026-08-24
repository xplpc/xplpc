interface NativeClient {
    call(data: string, callback: (response: string) => void): void;
}

interface NativePlatformProxy {
    initialize(): void;
}

// Embind exposes the bound class itself, so it is both constructable and extendable from javascript.
interface NativePlatformProxyClass {
    new (): NativePlatformProxy;
    extend(name: string, obj: unknown): NativePlatformProxyClass;
}

interface NativePlatformProxyList {
    prependFromJavascript: (item: NativePlatformProxy) => void;
}

interface NativeCallbackList {
    executeFromJavascript: (key: string, data: string) => void;
}

interface NativeLib {
    initialize(): void;
}

export interface WasmModule {
    XPLPC: NativeLib;
    Client: NativeClient;
    PlatformProxy: NativePlatformProxyClass;
    NativePlatformProxy: NativePlatformProxyClass;
    PlatformProxyList: NativePlatformProxyList;
    CallbackList: NativeCallbackList;

    // The emscripten runtime exports are enabled through EXPORTED_FUNCTIONS and EXPORTED_RUNTIME_METHODS.
    HEAPU8: Uint8Array<ArrayBuffer>;
    _malloc(size: number): number;
    _free(ptr: number): void;
}
