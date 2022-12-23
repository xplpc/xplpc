import { XWasmModule } from "@/xplpc/interface/wasm-module";
import { defineStore } from "pinia";

type WasmModuleState = {
    loaded: boolean
    module?: XWasmModule
}

export const useWasmModuleStore = defineStore("wasm-module", {
    state: (): WasmModuleState => ({
        loaded: false,
        module: undefined,
    }),
    getters: {
        isLoaded: (state) => state.loaded,
    },
    actions: {
        setLoaded(value: boolean) {
            this.loaded = value;
        },
    },
});
