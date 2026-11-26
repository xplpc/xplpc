import { XPLPC } from "../core/xplpc";
import { Request } from "../message/request";
import { ExceptionMessage } from "../util/exception-message";
import { Log } from "../util/log";

export class Client {
    static call<T>(request: Request): Promise<T | undefined>;

    static call(data: string): Promise<string>;

    static call<T>(request: Request | string): Promise<T | undefined | string> {
        const isRawRequest = typeof request === "string";
        const nothing = isRawRequest ? "" : undefined;

        return new Promise<T | undefined | string>((resolve) => {
            if (!XPLPC.shared().initialized) {
                Log.e("[Client : call] XPLPC was not initialized");
                return resolve(nothing);
            }

            const data = isRawRequest ? request : Client.data(request, "call");

            try {
                XPLPC.shared().module.JavascriptBridge.call(
                    data,
                    (response: string) => {
                        if (isRawRequest) {
                            return resolve(response);
                        }

                        return resolve(Client.decode<T>(response, "call"));
                    },
                );
            } catch (e: unknown) {
                Log.e("[Client : call] Error when reach the native side");
                Log.d(
                    "[Client : call] Error when reach the native side: " +
                        ExceptionMessage.get(e),
                );

                return resolve(nothing);
            }
        });
    }

    static callSync<T>(request: Request): T | undefined;

    static callSync(data: string): string;

    static callSync<T>(request: Request | string): T | undefined | string {
        const isRawRequest = typeof request === "string";

        // A request is written with the config, which the library only holds once it is up.
        if (!XPLPC.shared().initialized) {
            Log.e("[Client : callSync] XPLPC was not initialized");
            return isRawRequest ? "" : undefined;
        }

        const response = Client.answerSynchronously(
            isRawRequest ? request : Client.data(request, "callSync"),
        );

        if (response === undefined) {
            return isRawRequest ? "" : undefined;
        }

        if (isRawRequest) {
            return response;
        }

        return Client.decode<T>(response, "callSync");
    }

    // A serializer of the caller's own can raise while it writes the request, and the caller is answered rather than handed the exception.
    private static data(request: Request, source: string): string {
        try {
            return request.data();
        } catch (e: unknown) {
            Log.e("[Client : " + source + "] Error when encode the request");
            Log.d(
                "[Client : " +
                    source +
                    "] Error when encode the request: " +
                    ExceptionMessage.get(e),
            );
        }

        return "";
    }

    // A serializer that raises is answered as nothing, since the answer is decoded inside a native callback where nothing above the caller could catch it.
    private static decode<T>(response: string, source: string): T | undefined {
        try {
            return XPLPC.shared().config.serializer.decodeFunctionReturnValue<T>(
                response,
            );
        } catch (e: unknown) {
            Log.e("[Client : " + source + "] Error when decode data");
            Log.d(
                "[Client : " +
                    source +
                    "] Error when decode data: " +
                    ExceptionMessage.get(e),
            );
        }

        return undefined;
    }

    // A mapping that had not answered when the call returned is cancelled, so its later answer resolves nothing and leaves nothing behind.
    private static answerSynchronously(data: string): string | undefined {
        const bridge = XPLPC.shared().module.JavascriptBridge;
        const answer: { value?: string } = {};

        let call: bigint;

        try {
            call = bridge.call(data, (response: string) => {
                answer.value = response;
            });
        } catch (e: unknown) {
            Log.e("[Client : callSync] Error when reach the native side");
            Log.d(
                "[Client : callSync] Error when reach the native side: " +
                    ExceptionMessage.get(e),
            );

            return undefined;
        }

        if (call === 0n) {
            return answer.value;
        }

        bridge.cancel(call);
        Log.e("[Client : callSync] The function did not answer synchronously");

        return undefined;
    }
}
