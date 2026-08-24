export class Message {
    private data = new Map<string, unknown>();

    public get<T>(name: string): T | undefined {
        return this.data.get(name) as T | undefined;
    }

    public set(name: string, value: unknown) {
        this.data.set(name, value);
    }
}
