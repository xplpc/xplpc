import { XLocalClient as client } from "@/xplpc/client/local-client";
import { XWasmModule } from "@/xplpc/interface/wasm-module";
import Module from "@xplpc/build/wasm/bin/xplpc";
import { useWasmModuleStore } from "../store/wasm-module";

export default {
    install: () => {
        Module().then((wasmModule: XWasmModule) => {
            new Promise((r) => setTimeout(r, 300)).then(() => {
                const wm = useWasmModuleStore();
                wm.module = wasmModule;

                wm.module.XPLPC.initialize();
                wm.module.PlatformProxy.createDefault();
                wm.module.PlatformProxy.shared().initialize();

                client.wasmModule = wm.module;

                wm.setLoaded(true);
            });
        });
    },
};
