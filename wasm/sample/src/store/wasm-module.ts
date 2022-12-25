import { XPLPCModule } from "@/xplpc/module/xplpc";
import { defineStore } from "pinia";

type WasmModuleState = {
    loaded: boolean
    module?: XPLPCModule
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
