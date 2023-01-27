export class XMessage {
    private data = new Map<string, unknown>()

    public get<T>(name: string): T | undefined | unknown {
        if (this.data.has(name)) {
            return this.data.get(name);
        }

        return undefined
    }

    public set(name: string, value: unknown) {
        this.data.set(name, value);
    }
}
