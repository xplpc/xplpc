export class ExceptionMessage {
    public static get(e: unknown) {
        if (typeof e === "string") {
            return e.toUpperCase()
        } else if (e instanceof Error) {
            return e.message
        }

        return "unknown";
    }
}
