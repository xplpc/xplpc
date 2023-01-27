import { XMappingItem } from "../map/mapping-item";

export class XMappingList {
    private static instance: XMappingList;
    private list = new Map<string, XMappingItem>();

    private constructor() {
        // ignore
    }

    public static shared(): XMappingList {
        if (!XMappingList.instance) {
            XMappingList.instance = new XMappingList();
        }

        return XMappingList.instance;
    }

    public add(name: string, item: XMappingItem): void {
        this.list.set(name, item);
    }

    public find(name: string): XMappingItem | undefined {
        if (this.list.has(name)) {
            return this.list.get(name);
        }

        return undefined;
    }
}
