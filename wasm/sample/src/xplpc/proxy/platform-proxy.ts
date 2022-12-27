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
    doProxyCall: function (data: string): string {
        // TODO: XPLPC - NEED IMPLEMENT
        console.log("[XWebPlatformProxy : doProxyCall] Called: " + data);
        return '{"r": "test-ok"}';
    }
}

export default XWebPlatformProxy;
