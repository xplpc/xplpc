import { Mapping } from "@/custom/mapping";
import { Config } from "@/xplpc/core/config";
import { XPLPC } from "@/xplpc/core/xplpc";
import { WasmModule } from "@/xplpc/module/xplpc-module";
import { JsonSerializer } from "@/xplpc/serializer/json-serializer";
import { ExceptionMessage } from "@/xplpc/util/exception-message";
import { Log } from "@/xplpc/util/log";
import Module from "@xplpc/build/wasm/wasm32/bin/xplpc";
import { useWasmModuleStore } from "../store/wasm-module";

export default {
    install: () => {
        Module()
            .then((module: WasmModule) => {
                const wm = useWasmModuleStore();

                XPLPC.shared().initialize(
                    module,
                    new Config(new JsonSerializer()),
                );

                Mapping.initialize();

                wm.setLoaded(true);
            })
            .catch((e: unknown) => {
                Log.e("[WasmModule : install] Error when load the module");
                Log.d(
                    "[WasmModule : install] Error when load the module: " +
                        ExceptionMessage.get(e),
                );
            });
    },
};
