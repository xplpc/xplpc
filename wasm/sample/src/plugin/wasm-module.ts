import { Mapping } from "@/custom/mapping";
import { Config } from "@/xplpc/core/config";
import { XPLPC } from "@/xplpc/core/xplpc";
import { WasmModule } from "@/xplpc/module/xplpc-module";
import { JsonSerializer } from "@/xplpc/serializer/json-serializer";
import Module from "@xplpc/build/wasm/wasm32/bin/xplpc";
import { useWasmModuleStore } from "../store/wasm-module";

export default {
    install: () => {
        Module().then((module: WasmModule) => {
            const wm = useWasmModuleStore();

            XPLPC.shared().initialize(module, new Config(new JsonSerializer()));

            Mapping.initialize();

            wm.setLoaded(true);
        });
    },
};
