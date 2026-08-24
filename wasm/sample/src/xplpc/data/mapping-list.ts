import { MappingItem } from "../map/mapping-item";

export class MappingList {
    private static instance: MappingList;
    private list = new Map<string, MappingItem>();

    private constructor() {}

    public static shared(): MappingList {
        if (!MappingList.instance) {
            MappingList.instance = new MappingList();
        }

        return MappingList.instance;
    }

    public add(name: string, item: MappingItem): void {
        this.list.set(name, item);
    }

    public find(name: string): MappingItem | undefined {
        return this.list.get(name);
    }

    public has(name: string): boolean {
        return this.list.has(name);
    }

    public clear(): void {
        this.list.clear();
    }

    public count(): number {
        return this.list.size;
    }
}
