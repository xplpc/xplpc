interface IXNativePlatformProxyShared {
    shared: () => IXNativePlatformProxy;
}

interface IXNativeProxyClient {
    call(data: string, callback: (response: string) => void): string;
    callFromJavascript(data: string, callback: (response: string) => void): string;
}

interface IXNativePlatformProxy extends IXNativePlatformProxyShared {
    createFromPtr(proxy: IXNativePlatformProxy): void;
    createDefault(): void;
    initialize(): void;
    hasProxy(): boolean;
    extend(name: string, obj: unknown): unknown;
    callProxyCallback(key: string, data: string): void;
}

interface IXNativeLib {
    initialize(): void;
    isInitialized(): boolean;
}

export interface IXWasmModule {
    XPLPC: IXNativeLib,
    ProxyClient: IXNativeProxyClient,
    PlatformProxy: IXNativePlatformProxy
}
