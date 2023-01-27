import { XPLPC } from "../core/xplpc";
import { XMappingList } from "../data/mapping-list";
import { XMessage } from "../message/message";
import { XRequest } from "../message/request";
import { ExceptionMessage } from "../util/exception-message";
import { Log } from "../util/log";

export class XLocalClient {
    static call<T>(request: XRequest): Promise<T | undefined> {
        return new Promise<T | undefined>((resolve) => {
            const data = request.data();

            // function name
            const functionName = XPLPC.shared().config.serializer.decodeFunctionName(data);

            if (!functionName) {
                Log.e("[LocalClient : call] Function name is empty")
                return resolve(undefined);
            }

            // mapping item
            const mappingItem = XMappingList.shared().find(functionName);

            if (mappingItem == null) {
                Log.e("[LocalClient : call] Mapping not found for function: " + functionName)
                return resolve(undefined);
            }

            // execute
            let message: XMessage | undefined = undefined;

            try {
                message = XPLPC.shared().config.serializer.decodeMessage(data)
            } catch (e: unknown) {
                Log.e("[LocalClient : call] Error when decode message: " + ExceptionMessage.get(e))
            }

            if (!message) {
                Log.e("[LocalClient : call] Error when decode message for function: " + functionName)
                return resolve(undefined);
            }

            try {
                return resolve(mappingItem.target(message) as T | undefined);
            } catch (e: unknown) {
                Log.e("[LocalClient : call] Error: " + ExceptionMessage.get(e))
            }

            return resolve(undefined);
        });
    }
}
