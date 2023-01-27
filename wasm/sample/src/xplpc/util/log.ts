export class Log {
    static d(message?: unknown, ...optionalParams: unknown[]) {
        if (import.meta.env.DEV) {
            console.log(message, optionalParams);
        }
    }

    static e(message?: unknown, ...optionalParams: unknown[]) {
        console.error(message, optionalParams);
    }
}
