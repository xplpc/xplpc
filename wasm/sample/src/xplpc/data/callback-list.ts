import { Lock } from "../util/lock";

export interface XCallbackListItemType { (response: string): void }

export class XCallbackList {
    private static instance: XCallbackList;
    private list = new Map<string, XCallbackListItemType>();
    private lock = new Lock();

    private constructor() {
        // ignore
    }

    public static shared(): XCallbackList {
        if (!XCallbackList.instance) {
            XCallbackList.instance = new XCallbackList();
        }

        return XCallbackList.instance;
    }

    public async add(key: string, callback: XCallbackListItemType): Promise<void> {
        await this.lock.acquire();
        this.list.set(key, callback);
        return this.lock.release();
    }

    public async execute(key: string, data: string): Promise<void> {
        await this.lock.acquire();

        if (this.list.has(key)) {
            const callback = this.list.get(key);
            this.list.delete(key);
            this.lock.release();
            return callback?.(data);
        }

        return this.lock.release();
    }

    public async count(): Promise<number> {
        await this.lock.acquire();
        const amount = this.list.size;
        this.lock.release();
        return amount;
    }
}
