interface IXNativeClient {
    call(data: string, callback: (response: string) => void): string;
}

interface IXNativePlatformProxy {
    extend(name: string, obj: unknown): unknown;
    initialize(): void;
}

interface IXNativePlatformProxyList {
    shared: () => IXNativePlatformProxyList;
    appendFromJavascript: (item: IXNativePlatformProxy) => void;
    insertFromJavascript: (index: number, item: IXNativePlatformProxy) => void;
}

interface IXNativeCallbackList {
    executeFromJavascript: (key: string, data: string) => void;
}

interface IXNativeLib {
    initialize(): void;
    isInitialized(): boolean;
}

export interface IXWasmModule {
    XPLPC: IXNativeLib,
    Client: IXNativeClient,
    PlatformProxy: IXNativePlatformProxy,
    NativePlatformProxy: IXNativePlatformProxy,
    PlatformProxyList: IXNativePlatformProxyList,
    CallbackList: IXNativeCallbackList
}
