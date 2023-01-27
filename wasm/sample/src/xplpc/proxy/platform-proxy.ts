import { XProxyClient } from "../client/proxy-client";

interface IXWebPlatformProxy {
    __parent?: IXWebPlatformProxy;
    callProxy(key: string, data: string): void;
    callProxyCallback?(key: string, data: string): void;
}

const XWebPlatformProxy: IXWebPlatformProxy = {
    callProxy: function (key: string, data: string) {
        XProxyClient.call(data).then((response: string) => {
            this.__parent?.callProxyCallback?.call(this, key, response);
        });
    }
}

export default XWebPlatformProxy;
