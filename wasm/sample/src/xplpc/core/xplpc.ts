import { IXWasmModule } from "../module/xplpc-module";
import XWebPlatformProxy from "../proxy/platform-proxy";
import { XConfig } from "./config";

export class XPLPC {
    private static instance: XPLPC;

    public config!: XConfig;
    public module!: IXWasmModule;
    public initialized = false;

    private constructor() {
        // ignore
    }

    public static shared(): XPLPC {
        if (!XPLPC.instance) {
            XPLPC.instance = new XPLPC();
        }

        return XPLPC.instance;
    }

    public initialize(module: IXWasmModule, config: XConfig): void {
        if (this.initialized) {
            return;
        }

        this.initialized = true;
        this.config = config;

        // load native library
        this.module = module;

        if (this.module) {
            if (this.module.XPLPC) {
                this.module.XPLPC.initialize();
            }

            if (this.module.PlatformProxy) {
                // eslint-disable-next-line
                // @ts-ignore:next-line
                const ProxyClass = new this.module.PlatformProxy.extend("PlatformProxy", XWebPlatformProxy);
                const proxyInstance = new ProxyClass();

                this.module.PlatformProxy.createFromPtr(proxyInstance);
                this.module.PlatformProxy.shared().initialize();
            }
        }
    }
}
