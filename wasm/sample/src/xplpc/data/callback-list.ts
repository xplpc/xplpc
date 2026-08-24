export interface CallbackListItem {
    (response: string): void;
}

export class CallbackList {
    private static instance: CallbackList;
    private list = new Map<string, CallbackListItem>();

    private constructor() {}

    public static shared(): CallbackList {
        if (!CallbackList.instance) {
            CallbackList.instance = new CallbackList();
        }

        return CallbackList.instance;
    }

    public add(key: string, callback: CallbackListItem): void {
        this.list.set(key, callback);
    }

    public execute(key: string, data: string): void {
        // The wasm module answers synchronously, so registration and lookup must stay synchronous too.

        const callback = this.list.get(key);

        if (callback) {
            this.list.delete(key);
            callback(data);
        }
    }

    public remove(key: string): void {
        this.list.delete(key);
    }

    public clear(): void {
        this.list.clear();
    }

    public count(): number {
        return this.list.size;
    }
}
