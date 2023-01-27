import { Mapping } from "@/custom/mapping";
import { XConfig } from "@/xplpc/core/config";
import { XPLPC } from "@/xplpc/core/xplpc";
import { IXWasmModule } from "@/xplpc/module/xplpc-module";
import { JsonSerializer } from "@/xplpc/serializer/json-serializer";
import Module from "@xplpc/build/wasm/bin/xplpc";
import { useWasmModuleStore } from "../store/wasm-module";

export default {
    install: () => {
        Module().then((module: IXWasmModule) => {
            new Promise((r) => setTimeout(r, 300)).then(() => {
                // initialize module
                const wm = useWasmModuleStore();

                XPLPC.shared().initialize(module, new XConfig(new JsonSerializer()));

                // initialize local mappings
                Mapping.initialize();

                // define state
                wm.setLoaded(true);
            });
        });
    }
};
