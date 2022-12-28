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
    onRemoteProxyCall: function (data: string): Promise<string> {
        // TODO: XPLPC - NEED REAL IMPLEMENTATION
        console.log("[XWebPlatformProxy : onRemoteProxyCall] Called: " + data);

        const promise = new Promise<string>(function (resolve, reject) {
            // eslint-disable-next-line
            // @ts-ignore:next-line
            navigator.getBattery().then((battery: any) => {
                resolve('{"r": "' + (battery.level * 100) + '%"}');
            });
        });

        return promise;
    }
}

export default XWebPlatformProxy;
