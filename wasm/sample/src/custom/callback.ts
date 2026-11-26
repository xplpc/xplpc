import { Message } from "../xplpc/message/message";

export class Callback {
    public static batteryLevel(message: Message): Promise<string> {
        const suffix = message.get<string>("suffix") ?? "";

        return Promise.resolve("100" + suffix);
    }
}
