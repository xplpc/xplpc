import { defineStore } from "pinia";

type WasmModuleState = {
    loaded: boolean
}

export const useWasmModuleStore = defineStore("wasm-module", {
    state: (): WasmModuleState => ({
        loaded: false
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
