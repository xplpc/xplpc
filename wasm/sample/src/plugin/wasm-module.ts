import { XLocalClient } from "@/xplpc/client/local-client";
import { XProxyClient } from "@/xplpc/client/proxy-client";
import { XRemoteClient } from "@/xplpc/client/remote-client";
import { XPLPCModule } from "@/xplpc/module/xplpc";
import XWebPlatformProxy from "@/xplpc/proxy/platform-proxy";
import Module from "@xplpc/build/wasm/bin/xplpc";
import { useWasmModuleStore } from "../store/wasm-module";

export default {
    install: () => {
        Module().then((wasmModule: XPLPCModule) => {
            new Promise((r) => setTimeout(r, 300)).then(() => {
                const wm = useWasmModuleStore();
                wm.module = wasmModule;

                // eslint-disable-next-line
                // @ts-ignore:next-line
                const ProxyClass = new wm.module.PlatformProxy.extend("PlatformProxy", XWebPlatformProxy);
                const proxyInstance = new ProxyClass();

                wm.module.XPLPC.initialize();
                wm.module.PlatformProxy.createFromPtr(proxyInstance);
                wm.module.PlatformProxy.shared().initialize();

                XProxyClient.wasmModule = wm.module;
                XLocalClient.wasmModule = wm.module;
                XRemoteClient.wasmModule = wm.module;

                // TODO: XPLPC - REMOVE AFTER TEST
                // eslint-disable-next-line
                // @ts-ignore:next-line
                wm.module.HelloClass.SayHello("testeeee");

                wm.setLoaded(true);
            });
        });
    }
};
