import { XResponse } from "../message/response";

interface IXMappingItemTarget {
    (message: Map<string, unknown>): XResponse;
}

interface IXMappingItemExecutor {
    (value: string): string;
}

export class XMappingItem {
    name: string;
    target: IXMappingItemTarget;
    executor: IXMappingItemExecutor;

    constructor(name: string, target: IXMappingItemTarget, executor: IXMappingItemExecutor) {
        this.name = name;
        this.target = target;
        this.executor = executor;
    }
}
