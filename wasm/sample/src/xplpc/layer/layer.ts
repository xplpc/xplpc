import { Config } from "../core/config";
import { MappingList } from "../data/mapping-list";
import { DecodedRequest } from "../message/decoded-request";
import { HostLayer, WasmModule } from "../module/xplpc-module";
import { ExceptionMessage } from "../util/exception-message";
import { Log } from "../util/log";

// The page as one layer of the router, which serves a call for a name it declared.
export class Layer implements HostLayer {
    constructor(
        private readonly module: WasmModule,
        private readonly config: Config,
    ) {}

    invoke(call: bigint, data: string): void {
        // A serializer is code this library may not have written, and the call it fails to read is answered rather than left waiting.
        let request: DecodedRequest | undefined;

        try {
            request = this.config.serializer.decodeRequest(data);
        } catch (e: unknown) {
            Log.e("[Layer : invoke] Error when decode request");
            Log.d(
                "[Layer : invoke] Error when decode request: " +
                    ExceptionMessage.get(e),
            );
            this.answer(call, "");
            return;
        }

        if (!request) {
            this.answer(call, "");
            return;
        }

        const functionName = request.functionName;
        const mappingItem = MappingList.shared().find(functionName);

        if (!mappingItem) {
            Log.e(
                "[Layer : invoke] Mapping not found for function: " +
                    functionName,
            );
            this.answer(call, "");
            return;
        }

        // A mapping that raises before returning a promise and one whose promise rejects are the same failure, so both reach the one handler below.
        let pending: Promise<unknown>;

        try {
            pending = Promise.resolve(mappingItem.target(request.message));
        } catch (e: unknown) {
            pending = Promise.reject(e);
        }

        pending.then(
            (response: unknown) => {
                this.answer(call, this.encode(functionName, response));
            },
            (e: unknown) => {
                Log.e(
                    '[Layer : invoke] Error when execute function "' +
                        functionName +
                        '"',
                );
                Log.d(
                    '[Layer : invoke] Error when execute function "' +
                        functionName +
                        '": ' +
                        ExceptionMessage.get(e),
                );
                this.answer(call, "");
            },
        );
    }

    // The answer is written in a later microtask, where what a serializer raises would otherwise leave the caller waiting.
    private encode(functionName: string, value: unknown): string {
        try {
            return this.config.serializer.encodeFunctionReturnValue(value);
        } catch (e: unknown) {
            Log.e(
                '[Layer : invoke] Error when encode the answer of function "' +
                    functionName +
                    '"',
            );
            Log.d(
                '[Layer : invoke] Error when encode the answer of function "' +
                    functionName +
                    '": ' +
                    ExceptionMessage.get(e),
            );

            return "";
        }
    }

    private answer(call: bigint, data: string): void {
        this.module.JavascriptBridge.answer(call, data);
    }
}
