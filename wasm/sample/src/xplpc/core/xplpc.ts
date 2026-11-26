import { Layer } from "../layer/layer";
import { WasmModule } from "../module/xplpc-module";
import { Config } from "./config";

export class XPLPC {
    private static instance: XPLPC;

    public config!: Config;
    public module!: WasmModule;
    public initialized = false;

    private constructor() {}

    public static shared(): XPLPC {
        if (!XPLPC.instance) {
            XPLPC.instance = new XPLPC();
        }

        return XPLPC.instance;
    }

    public initialize(module: WasmModule, config: Config): void {
        if (this.initialized) {
            return;
        }

        // The module attaches the layer only once the mappings c++ registers came up, so a failure here leaves nothing behind for a retry.
        module.JavascriptBridge.attach(new Layer(module, config));

        this.config = config;
        this.module = module;
        this.initialized = true;
    }
}
