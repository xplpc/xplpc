interface XPlatformProxyShared {
    shared: () => XPlatformProxy;
}

export interface XPlatformProxy extends XPlatformProxyShared {
    createDefault(): void;
    initialize(): void;
}
