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
// The reviver rejects that value instead of handing back a different one.
function rejectRoundedIntegers(
    key: string,
    value: unknown,
    context?: JsonParseContext,
): unknown {
    if (typeof value !== "number" || Number.isSafeInteger(value)) {
        return value;
    }

    // A runtime that hands over no source text leaves only the number, and any whole value past the safe range may be an integer it already rounded.
    const source = context?.source;
    const rounded =
        source === undefined ? Number.isInteger(value) : /^-?\d+$/.test(source);

    if (!rounded) {
        return value;
    }

    throw new RangeError(
        `The value ${source ?? value} of "${key}" needs more precision than a javascript number holds`,
    );
}

interface DocumentData {
    f?: unknown;
    p?: unknown;
    n?: unknown;
    v?: unknown;
    r?: unknown;
}

function isObject(value: unknown): value is DocumentData {
    return typeof value === "object" && value !== null && !Array.isArray(value);
}

function parse(data: string): unknown {
    return JSON.parse(data, rejectRoundedIntegers);
}

// JSON.stringify writes a date as text and a map, a set or binary data in a shape of its own, none of which the wire carries, so each is refused rather than written wrong.
function refuseUnwritable(this: unknown, key: string, value: unknown): unknown {
    const original =
        typeof this === "object" && this !== null
            ? Reflect.get(this, key)
            : value;

    if (
        original instanceof Date ||
        original instanceof Map ||
        original instanceof Set ||
        original instanceof ArrayBuffer ||
        ArrayBuffer.isView(original)
    ) {
        throw new TypeError(
            `The value of "${key}" cannot be written to the wire`,
        );
    }

    return value;
}

function stringify(document: object): string {
    return JSON.stringify(document, refuseUnwritable);
}

export class JsonSerializer implements Serializer {
    decodeRequest(data: string): DecodedRequest | undefined {
        try {
            const decodedData = parse(data);

            if (!isObject(decodedData)) {
                Log.e(
                    "[JsonSerializer : decodeRequest] Error when decode request",
                );
                return undefined;
            }

            // A request that names no function as text routes nowhere, which is what the reference refuses before it ever reaches a decoder.
            const functionName = decodedData.f;

            if (typeof functionName !== "string" || functionName === "") {
                Log.e(
                    "[JsonSerializer : decodeRequest] The request names no function",
                );
                return undefined;
            }

            const params = decodedData.p ?? [];

            if (!Array.isArray(params)) {
                Log.e(
                    '[JsonSerializer : decodeRequest] Parameters of function "' +
                        functionName +
                        '" are not a list',
                );
                return undefined;
            }

            const message = new Message();
            const names = new Set<string>();

            for (const param of params) {
                if (!isObject(param) || typeof param.n !== "string") {
                    Log.e(
                        '[JsonSerializer : decodeRequest] Function "' +
                            functionName +
                            '" carries a parameter whose name is not text',
                    );
                    return undefined;
                }

                // A name read twice means the first value to one bridge and the last to another, so the request is refused rather than read either way.
                if (names.has(param.n)) {
                    Log.e(
                        '[JsonSerializer : decodeRequest] Parameter "' +
                            param.n +
                            '" of function "' +
                            functionName +
                            '" appears more than once',
                    );
                    return undefined;
                }

                names.add(param.n);
                message.set(param.n, param.v);
            }

            return new DecodedRequest(functionName, message);
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
            // A mapping with nothing to answer travels as null, like a parameter without a value.
            return stringify({
                r: data ?? null,
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
            const decodedData = parse(data);

            return isObject(decodedData)
                ? (decodedData.r as T | undefined)
                : undefined;
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
            return stringify({
                f: functionName,
                p: params.map((item) => {
                    return { n: item.name, v: item.value ?? null };
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
