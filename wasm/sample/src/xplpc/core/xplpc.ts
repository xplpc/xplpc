import { WasmModule } from "../module/xplpc-module";
import platformProxy from "../proxy/platform-proxy";
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

        this.config = config;
        this.module = module;

        const nativePlatformProxy = new module.NativePlatformProxy();
        nativePlatformProxy.initialize();
        module.PlatformProxyList.prependFromJavascript(nativePlatformProxy);

        const ProxyClass = module.PlatformProxy.extend(
            "PlatformProxy",
            platformProxy,
        );
        const webProxy = new ProxyClass();
        webProxy.initialize();
        module.PlatformProxyList.prependFromJavascript(webProxy);

        module.XPLPC.initialize();

        // This is only set once the module is fully wired, since a failure above must not leave a half built singleton.
        this.initialized = true;
    }
}
