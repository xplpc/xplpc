import { WasmModule } from "../src/xplpc/module/xplpc-module";

declare module "@xplpc/build/wasm/wasm32/bin/xplpc" {
    // The module writes what the native side logs through print, which a page can take over to read those lines itself.
    export default function Module(options?: {
        print?: (line: string) => void;
    }): Promise<WasmModule>;
}
