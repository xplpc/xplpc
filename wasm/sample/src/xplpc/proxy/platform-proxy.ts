import { XPLPC } from "../core/xplpc";
import { XCallbackList } from "../data/callback-list";
import { XMappingList } from "../data/mapping-list";
import { XMessage } from "../message/message";
import { ExceptionMessage } from "../util/exception-message";
import { Log } from "../util/log";

interface IXWebPlatformProxy {
    initialize(): void;
    initializePlatform(): void;
    finalize(): void;
    finalizePlatform(): void;
    callProxy(key: string, data: string): void;
    callProxyCallback(key: string, data: string): void;
    hasMapping(name: string): boolean;
}

const XWebPlatformProxy: IXWebPlatformProxy = {
    initialize() {
        this.initializePlatform();
    },
    initializePlatform() {
        // ignore
    },
    finalize() {
        this.finalizePlatform();
    },
    finalizePlatform() {
        // ignore
    },
    callProxy: function (key: string, data: string) {
        if (!XPLPC.shared().initialized) {
            Log.e("[XWebPlatformProxy : call] The WASM module is not initialized");
            return;
        }

        // function name
        const functionName = XPLPC.shared().config.serializer.decodeFunctionName(data)

        if (!functionName) {
            Log.e("[XWebPlatformProxy : call] Function name is empty");
            XPLPC.shared().module.CallbackList.executeFromJavascript(key, "");
            return;
        }

        // mapping item
        const mappingItem = XMappingList.shared().find(functionName)

        if (!mappingItem) {
            Log.e("[XWebPlatformProxy : call] Mapping not found for function: " + functionName);
            XPLPC.shared().module.CallbackList.executeFromJavascript(key, "");
            return;
        }

        // execute
        let message: XMessage | undefined = undefined;

        try {
            message = XPLPC.shared().config.serializer.decodeMessage(data);
        } catch (e: unknown) {
            Log.e("[XWebPlatformProxy : call] Error when decode message: " + ExceptionMessage.get(e));
        }

        if (!message) {
            Log.e("[XWebPlatformProxy : call] Error when decode message for function: " + functionName);
            XPLPC.shared().module.CallbackList.executeFromJavascript(key, "");
            return;
        }

        mappingItem.target(message)
            .then((response: unknown) => {
                return XPLPC.shared().config.serializer.encodeFunctionReturnValue(response);
            }).then((data: string) => {
                XPLPC.shared().module.CallbackList.executeFromJavascript(key, data);
            }).catch((e: Error) => {
                Log.e("[XWebPlatformProxy : call] Error when encode message: " + ExceptionMessage.get(e))
                XPLPC.shared().module.CallbackList.executeFromJavascript(key, "");
            });
    },
    callProxyCallback(key: string, data: string) {
        XCallbackList.shared().execute(key, data);
    },
    hasMapping(name: string): boolean {
        const mappingItem = XMappingList.shared().find(name);

        if (mappingItem) {
            return true;
        }

        return false;
    }
}

export default XWebPlatformProxy;
