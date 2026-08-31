import { XPLPC } from "../core/xplpc";
import { MappingList } from "../data/mapping-list";
import { ExceptionMessage } from "../util/exception-message";
import { Log } from "../util/log";

interface PlatformProxy {
    initialize(): void;
    initializePlatform(): void;
    callProxy(key: string, data: string): void;
    hasMapping(name: string): boolean;
}

function callNativeProxyCallback(key: string, data: string): void {
    XPLPC.shared().module.CallbackList.executeFromJavascript(key, data);
}

const platformProxy: PlatformProxy = {
    initialize() {
        this.initializePlatform();
    },

    initializePlatform() {},

    callProxy(key: string, data: string) {
        if (!XPLPC.shared().initialized) {
            Log.e("[PlatformProxy : call] XPLPC was not initialized");
            callNativeProxyCallback(key, "");
            return;
        }

        const request = XPLPC.shared().config.serializer.decodeRequest(data);

        if (!request) {
            callNativeProxyCallback(key, "");
            return;
        }

        if (!request.functionName) {
            Log.e("[PlatformProxy : call] Function name is empty");
            callNativeProxyCallback(key, "");
            return;
        }

        const mappingItem = MappingList.shared().find(request.functionName);

        if (!mappingItem) {
            Log.e(
                "[PlatformProxy : call] Mapping not found for function: " +
                    request.functionName,
            );
            callNativeProxyCallback(key, "");
            return;
        }

        mappingItem
            .target(request.message)
            .then((response: unknown) => {
                callNativeProxyCallback(
                    key,
                    XPLPC.shared().config.serializer.encodeFunctionReturnValue(
                        response,
                    ),
                );
            })
            .catch((e: unknown) => {
                Log.e(
                    '[PlatformProxy : call] Error when execute function "' +
                        request.functionName +
                        '"',
                );
                Log.d(
                    '[PlatformProxy : call] Error when execute function "' +
                        request.functionName +
                        '": ' +
                        ExceptionMessage.get(e),
                );
                callNativeProxyCallback(key, "");
            });
    },

    hasMapping(name: string): boolean {
        return MappingList.shared().has(name);
    },
};

export default platformProxy;
