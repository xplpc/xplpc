import { XMessage } from "../message/message";

interface IXMappingItemTarget {
    (message: XMessage): Promise<unknown>;
}

export class XMappingItem {
    target: IXMappingItemTarget;

    constructor(target: IXMappingItemTarget) {
        this.target = target;
    }
}
