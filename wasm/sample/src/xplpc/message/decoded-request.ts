import { Message } from "./message";

export class DecodedRequest {
    constructor(
        readonly functionName: string,
        readonly message: Message,
    ) {}
}
