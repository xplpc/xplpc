import { XPLPC } from "../core/xplpc";
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

    // The mapping is recorded before the name is declared, so a call routed the moment the router hears of it already finds it.
    public add(name: string, item: MappingItem): void {
        if (!XPLPC.shared().initialized) {
            throw new Error(
                "XPLPC is not initialized, so a mapping cannot be declared",
            );
        }

        const previous = this.list.get(name);
        this.list.set(name, item);

        try {
            XPLPC.shared().module.JavascriptBridge.declare(name);
        } catch (e: unknown) {
            if (previous) {
                this.list.set(name, previous);
            } else {
                this.list.delete(name);
            }

            throw e;
        }
    }

    public find(name: string): MappingItem | undefined {
        return this.list.get(name);
    }

    public has(name: string): boolean {
        return this.list.has(name);
    }

    public clear(): void {
        // Each name leaves the list as the router forgets it, so a retract that fails partway leaves the two agreeing.
        for (const name of [...this.list.keys()]) {
            XPLPC.shared().module.JavascriptBridge.retract(name);
            this.list.delete(name);
        }
    }

    public count(): number {
        return this.list.size;
    }
}
