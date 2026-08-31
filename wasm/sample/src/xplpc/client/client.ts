import { XPLPC } from "../core/xplpc";
import { CallbackList } from "../data/callback-list";
import { Request } from "../message/request";
import { ExceptionMessage } from "../util/exception-message";
import { Log } from "../util/log";
import { UniqueID } from "../util/unique-id";

export class Client {
    static call<T>(request: Request): Promise<T | undefined>;

    static call(data: string): Promise<string>;

    static call<T>(
        request: Request | string,
    ): Promise<T | undefined> | Promise<string> {
        return new Promise<T | undefined>((resolve) => {
            if (!XPLPC.shared().initialized) {
                Log.e("[Client : call] The WASM module is not initialized");
                return resolve(undefined);
            }

            const isRawRequest = typeof request === "string";
            const key = UniqueID.generate();

            CallbackList.shared().add(key, (response: string) => {
                if (isRawRequest) {
                    return resolve(response as T | undefined);
                }

                return resolve(
                    XPLPC.shared().config.serializer.decodeFunctionReturnValue<T>(
                        response,
                    ),
                );
            });

            try {
                XPLPC.shared().module.Client.call(
                    isRawRequest ? request : request.data(),
                    (response: string) => {
                        CallbackList.shared().execute(key, response);
                    },
                );
            } catch (e: unknown) {
                Log.e("[Client : call] Error when reach the native side");
                Log.d(
                    "[Client : call] Error when reach the native side: " +
                        ExceptionMessage.get(e),
                );

                CallbackList.shared().remove(key);

                return resolve(undefined);
            }
        });
    }

    static callSync<T>(request: Request): T | undefined;

    static callSync(data: string): string;

    static callSync<T>(request: Request | string): T | undefined | string {
        const isRawRequest = typeof request === "string";

        const response = Client.answerSynchronously(
            isRawRequest ? request : request.data(),
        );

        if (response === undefined) {
            return isRawRequest ? "" : undefined;
        }

        if (isRawRequest) {
            return response;
        }

        return XPLPC.shared().config.serializer.decodeFunctionReturnValue<T>(
            response,
        );
    }

    private static answerSynchronously(data: string): string | undefined {
        if (!XPLPC.shared().initialized) {
            Log.e("[Client : callSync] The WASM module is not initialized");
            return undefined;
        }

        // A page has one thread, so nothing can write the answer while this function reads it.
        const key = UniqueID.generate();
        const answer: { value?: string } = {};

        CallbackList.shared().add(key, (response: string) => {
            answer.value = response;
        });

        try {
            XPLPC.shared().module.Client.call(data, (response: string) => {
                CallbackList.shared().execute(key, response);
            });
        } catch (e: unknown) {
            Log.e("[Client : callSync] Error when reach the native side");
            Log.d(
                "[Client : callSync] Error when reach the native side: " +
                    ExceptionMessage.get(e),
            );

            CallbackList.shared().remove(key);

            return undefined;
        }

        // Taking the key back is what decides the two cases, since a mapping that answered inline has already taken it and one that deferred never will.
        CallbackList.shared().remove(key);

        if (answer.value === undefined) {
            Log.e(
                "[Client : callSync] The function did not answer synchronously",
            );
        }

        return answer.value;
    }
}
