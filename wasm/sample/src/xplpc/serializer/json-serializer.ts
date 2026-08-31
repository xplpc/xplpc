import { DecodedRequest } from "../message/decoded-request";
import { Message } from "../message/message";
import { Param } from "../message/param";
import { ExceptionMessage } from "../util/exception-message";
import { Log } from "../util/log";
import { Serializer } from "./serializer";

interface JsonParseContext {
    source?: string;
}

// The runtime hands the reviver the original text, which the standard library types do not describe yet.
declare global {
    interface JSON {
        parse(
            text: string,
            reviver: (
                this: unknown,
                key: string,
                value: unknown,
                context?: JsonParseContext,
            ) => unknown,
        ): unknown;
    }
}

// A javascript number is a double, so an integer literal above the safe range is rounded on parse.
// The reviver sees the original text and rejects the value instead of handing back a different one.
function rejectRoundedIntegers(
    key: string,
    value: unknown,
    context?: JsonParseContext,
): unknown {
    const source = context?.source;

    if (
        typeof value !== "number" ||
        source === undefined ||
        !/^-?\d+$/.test(source) ||
        Number.isSafeInteger(value)
    ) {
        return value;
    }

    throw new RangeError(
        `The value ${source} of "${key}" needs more precision than a javascript number holds`,
    );
}

interface RequestData {
    f?: string;
    p?: { n: string; v: unknown }[];
}

interface ResponseData {
    r?: unknown;
}

function parse<T>(data: string): T {
    return JSON.parse(data, rejectRoundedIntegers) as T;
}

export class JsonSerializer implements Serializer {
    decodeRequest(data: string): DecodedRequest | undefined {
        try {
            const decodedData = parse<RequestData>(data);
            const message = new Message();

            decodedData.p?.forEach((param) => {
                message.set(param.n, param.v);
            });

            return new DecodedRequest(decodedData.f ?? "", message);
        } catch (e: unknown) {
            Log.e("[JsonSerializer : decodeRequest] Error when decode request");
            Log.d(
                "[JsonSerializer : decodeRequest] Error when decode request: " +
                    ExceptionMessage.get(e),
            );
        }

        return undefined;
    }

    encodeFunctionReturnValue(data: unknown): string {
        try {
            return JSON.stringify({
                r: data,
            });
        } catch (e: unknown) {
            Log.e(
                "[JsonSerializer : encodeFunctionReturnValue] Error when encode data",
            );
            Log.d(
                "[JsonSerializer : encodeFunctionReturnValue] Error when encode data: " +
                    ExceptionMessage.get(e),
            );
        }

        return "";
    }

    decodeFunctionReturnValue<T>(data: string): T | undefined {
        // An empty response is the empty value every failing path answers with, not a document that failed to parse.
        if (!data) {
            return undefined;
        }

        try {
            return parse<ResponseData>(data).r as T | undefined;
        } catch (e: unknown) {
            Log.e(
                "[JsonSerializer : decodeFunctionReturnValue] Error when parse json",
            );
            Log.d(
                "[JsonSerializer : decodeFunctionReturnValue] Error when parse json: " +
                    ExceptionMessage.get(e),
            );
        }

        return undefined;
    }

    encodeRequest(functionName: string, ...params: Param[]): string {
        try {
            return JSON.stringify({
                f: functionName,
                p: params.map((item) => {
                    return { n: item.name, v: item.value };
                }),
            });
        } catch (e: unknown) {
            Log.e("[JsonSerializer : encodeRequest] Error when encode data");
            Log.d(
                "[JsonSerializer : encodeRequest] Error when encode data: " +
                    ExceptionMessage.get(e),
            );
        }

        return "";
    }
}
