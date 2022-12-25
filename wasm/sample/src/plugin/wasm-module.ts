import { XLocalClient } from "@/xplpc/client/local-client";
import { XProxyClient } from "@/xplpc/client/proxy-client";
import { XRemoteClient } from "@/xplpc/client/remote-client";
import { XPLPCModule } from "@/xplpc/module/xplpc";
import { XWebPlatformProxy } from "@/xplpc/proxy/platform-proxy";
import Module from "@xplpc/build/wasm/bin/xplpc";
import { useWasmModuleStore } from "../store/wasm-module";

export default {
    install: () => {
        Module().then((wasmModule: XPLPCModule) => {
            new Promise((r) => setTimeout(r, 300)).then(() => {
                const wm = useWasmModuleStore();
                wm.module = wasmModule;

                wm.module.XPLPC.initialize();

                wm.module.PlatformProxy.create(new XWebPlatformProxy());
                wm.module.PlatformProxy.shared().initialize();

                XProxyClient.wasmModule = wm.module;
                XLocalClient.wasmModule = wm.module;
                XRemoteClient.wasmModule = wm.module;

                //new XWebPlatformClient().call("{}");

                // wm.module.MappingData.add(new XMappingItem("platform.battery.level", CustomFunctions.batteryLevel, (value: string): string => {
                //     console.log("Value: ", value);
                //     return "{}";
                // }));

                // wm.module.MappingData.add(new XMappingItem("platform.battery.level", CustomFunctions.batteryLevel, (value: string): string => {
                //     console.log("Value: ", value);
                //     return "{}";
                // }));

                wm.setLoaded(true);
            });
        });
    }
};
