export class Log {
    // The browser console has no registry to name a logger in, so the group every line carries is what lets a consumer filter this library out of its own.
    private static readonly group = "[XPLPC]";

    static d(message?: unknown, ...optionalParams: unknown[]) {
        if (import.meta.env.DEV) {
            console.log(Log.group, message, ...optionalParams);
        }
    }

    static e(message?: unknown, ...optionalParams: unknown[]) {
        console.error(Log.group, message, ...optionalParams);
    }
}
