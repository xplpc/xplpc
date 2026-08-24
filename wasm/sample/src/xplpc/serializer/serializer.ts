import { DecodedRequest } from "../message/decoded-request";
import { Param } from "../message/param";

export interface Serializer {
    decodeRequest(data: string): DecodedRequest | undefined;
    encodeFunctionReturnValue(data: unknown): string;
    decodeFunctionReturnValue<T>(data: string): T | undefined;
    encodeRequest(functionName: string, ...params: Param[]): string;
}
