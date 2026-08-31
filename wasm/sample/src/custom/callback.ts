import { Message } from "../xplpc/message/message";

export class Callback {
    public static batteryLevel(message: Message): Promise<string> {
        return new Promise<string>((resolve) => {
            const suffix = message.get("suffix");
            return resolve("100" + suffix);
        });
    }
}
