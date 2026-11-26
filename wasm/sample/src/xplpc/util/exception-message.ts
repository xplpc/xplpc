export class ExceptionMessage {
    public static get(e: unknown) {
        if (typeof e === "string") {
            return e;
        }

        if (e instanceof Error) {
            return e.message;
        }

        return "unknown";
    }
}
