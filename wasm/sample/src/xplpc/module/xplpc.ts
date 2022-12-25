import { IXNativeProxyClient } from "../client/proxy-client";
import { XPLPC } from "../core/xplpc";
import { IXNativePlatformProxy } from "../proxy/platform-proxy";

export interface XPLPCModule {
    XPLPC: XPLPC,
    ProxyClient: IXNativeProxyClient,
    PlatformProxy: IXNativePlatformProxy
}
