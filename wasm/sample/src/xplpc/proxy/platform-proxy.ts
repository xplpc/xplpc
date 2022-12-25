import { XProxyClient } from "../client/proxy-client";

interface IXPlatformProxy {
    call(data: string): string;
}

interface IXNativePlatformProxyShared {
    shared: () => IXNativePlatformProxy;
}

export interface IXNativePlatformProxy extends IXNativePlatformProxyShared {
    createDefault(): void;
    initialize(): void;
    create(proxy: IXPlatformProxy): void;
    hasProxy(): boolean;
}

export class XWebPlatformClient implements IXPlatformProxy {
    call(data: string): string {
        return XProxyClient.call(data);
    }
}
