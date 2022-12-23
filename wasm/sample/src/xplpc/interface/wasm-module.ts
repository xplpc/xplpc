import { XProxyClient } from "../client/proxy-client";
import { XPLPC } from "../core/xplpc";
import { XPlatformProxy } from "../proxy/platform-proxy";

export interface XWasmModule {
    XPLPC: XPLPC,
    ProxyClient: XProxyClient,
    PlatformProxy: XPlatformProxy
}
