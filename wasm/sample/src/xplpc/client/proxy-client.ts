import { XPLPC } from "../core/xplpc";
import { XMappingList } from "../data/mapping-list";
import { XMessage } from "../message/message";
import { ExceptionMessage } from "../util/exception-message";
import { Log } from "../util/log";

export class XProxyClient {
    static call(data: string): Promise<string> {
        return new Promise<string>((resolve) => {
            if (!XPLPC.shared().initialized) {
                Log.e("[XProxyClient : call] The WASM module is not initialized");
                return resolve("");
            }

            // function name
            const functionName = XPLPC.shared().config.serializer.decodeFunctionName(data)

            if (!functionName) {
                Log.e("[ProxyClient : call] Function name is empty")
                return resolve("");
            }

            // mapping item
            const mappingItem = XMappingList.shared().find(functionName)

            if (!mappingItem) {
                Log.e("[ProxyClient : call] Mapping not found for function: " + functionName)
                return resolve("");
            }

            // execute
            let message: XMessage | undefined = undefined;

            try {
                message = XPLPC.shared().config.serializer.decodeMessage(data)
            } catch (e: unknown) {
                Log.e("[ProxyClient : call] Error when decode message: " + ExceptionMessage.get(e))
            }

            if (!message) {
                Log.e("[ProxyClient : call] Error when decode message for function: " + functionName)
                return resolve("");
            }

            try {
                return resolve(mappingItem.target(message).then((response: unknown) => {
                    return XPLPC.shared().config.serializer.encodeFunctionReturnValue(response)
                }));
            } catch (e: unknown) {
                Log.e("[ProxyClient : call] Error when encode message: " + ExceptionMessage.get(e))
                return resolve("");
            }
        });
    }
}
