interface IXNativePlatformProxyShared {
    shared: () => IXNativePlatformProxy;
}

export interface IXNativePlatformProxy extends IXNativePlatformProxyShared {
    createFromPtr(proxy: IXNativePlatformProxy): void;
    createDefault(): void;
    initialize(): void;
    hasProxy(): boolean;
    extend(name: string, obj: unknown): unknown;
}

const XWebPlatformProxy = {
    sleep: function (time: number) {
        return new Promise((resolve) => setTimeout(resolve, time));
    },
    onRemoteProxyCall: function (data: string): string {
        // TODO: XPLPC - NEED REAL IMPLEMENTATION
        console.log("[XWebPlatformProxy : onRemoteProxyCall] Called: " + data);
        return '{"r": "sync-test-ok"}';
    },
    // TODO: XPLPC - WHAT IS THE CORRECT METHOD SIGNATURE FOR ASYNC AND CALL THE CALLBACK?
    onRemoteProxyCallAsync: async function (data: string, callback: (response: string) => void) {
        // TODO: XPLPC - THIS CODE IS NEVER CALLED
        // TODO: XPLPC - NEED REAL IMPLEMENTATION
        console.log("[XWebPlatformProxy : onRemoteProxyCallAsync] Called: " + data);

        callback('{"r": "async-test-ok"}');
    }
}

export default XWebPlatformProxy;
