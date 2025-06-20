import { IXWasmModule } from "../src/xplpc/module/xplpc-module";

declare module "@xplpc/build/wasm/wasm32/bin/xplpc" {
  export default function Module(): Promise<IXWasmModule>;
}
