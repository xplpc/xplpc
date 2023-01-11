import { XResponse } from "../message/response";

export class CustomFunctions {
    static batteryLevel(message: Map<string, unknown>): XResponse {
        console.log("Message: " + message);
        return new XResponse("123 %");
    }
}
