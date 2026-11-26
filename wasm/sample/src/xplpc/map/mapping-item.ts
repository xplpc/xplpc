import { Message } from "../message/message";

interface MappingItemTarget {
    (message: Message): Promise<unknown>;
}

export class MappingItem {
    target: MappingItemTarget;

    constructor(target: MappingItemTarget) {
        this.target = target;
    }
}
