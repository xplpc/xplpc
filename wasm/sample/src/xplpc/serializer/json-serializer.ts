import { XMessage } from "../message/message";
import { XParam } from "../message/param";
import { ExceptionMessage } from "../util/exception-message";
import { Log } from "../util/log";
import { IXSerializer } from "./serializer";

export class JsonSerializer implements IXSerializer {
    decodeFunctionName(data: string): string {
        try {
            return JSON.parse(data).f;
        } catch (e: unknown) {
            Log.e("[JsonSerializer : decodeFunctionName] Error when parse json: " + ExceptionMessage.get(e));
        }

        return "";
    }

    decodeMessage(data: string): XMessage | undefined {
        try {
            // decode parameters
            const decodedData = JSON.parse(data);

            // message data
            const message = new XMessage();

            decodedData.p.forEach((param: { n: string, v: unknown }) => {
                message.set(param.n, param.v);
            });

            return message;
        } catch (e: unknown) {
            Log.e("[JsonSerializer : decodeMessage] Error when decode message: " + ExceptionMessage.get(e));
        }

        return undefined;
    }

    encodeFunctionReturnValue(data: unknown): string {
        try {
            return JSON.stringify({
                r: data
            });
        } catch (e: unknown) {
            Log.e("[JsonSerializer : encodeFunctionReturnValue] Error when encode data: " + ExceptionMessage.get(e));
        }

        return "";
    }

    decodeFunctionReturnValue<T>(data: string): T | undefined {
        try {
            return JSON.parse(data).r;
        } catch (e: unknown) {
            Log.e("[JsonSerializer : decodeFunctionReturnValue] Error when parse json: " + ExceptionMessage.get(e));
        }

        return undefined;
    }

    encodeRequest(functionName: string, ...params: XParam[]): string {
        try {
            return JSON.stringify({
                f: functionName,
                p: params.map(item => {
                    return { n: item.name, v: item.value };
                })
            });
        } catch (e: unknown) {
            Log.e("[JsonSerializer : encodeRequest] Error when encode data: " + ExceptionMessage.get(e));
        }

        return "";
    }
}
