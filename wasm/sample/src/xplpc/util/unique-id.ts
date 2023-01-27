import { Lock } from "../util/lock";

export class UniqueID {
    private static instance: UniqueID;
    private lock = new Lock();
    private uid = 0;

    private constructor() {
        // ignore
    }

    public static shared(): UniqueID {
        if (!UniqueID.instance) {
            UniqueID.instance = new UniqueID();
        }

        return UniqueID.instance;
    }

    static async generate(): Promise<string> {
        await UniqueID.shared().lock.acquire();
        const value = (++UniqueID.shared().uid).toString();
        UniqueID.shared().lock.release();
        return value;
    }
}
