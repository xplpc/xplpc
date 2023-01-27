import { XMessage } from "../message/message";
import { XParam } from "../message/param";

export interface IXSerializer {
    decodeFunctionName(data: string): string;
    decodeMessage(data: string): XMessage | undefined;
    encodeFunctionReturnValue(data: unknown): string;
    decodeFunctionReturnValue<T>(data: string): T | undefined;
    encodeRequest(functionName: string, ...params: XParam[]): string;
}
