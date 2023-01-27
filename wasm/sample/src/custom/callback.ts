import { XMessage } from "../xplpc/message/message";

export class Callback {
    public static batteryLevel(message: XMessage): Promise<string> {
        return new Promise<string>((resolve) => {
            const suffix = message.get("suffix");
            return resolve("100" + suffix);
        });
    }
}
