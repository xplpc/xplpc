import { XParam } from "./param";

export class XRequest {
    functionName: string;
    params: XParam[];

    constructor(functionName: string, ...params: XParam[]) {
        this.functionName = functionName;
        this.params = params;
    }

    data(): string {
        return JSON.stringify({
            f: this.functionName,
            p: this.params.map(item => {
                return { n: item.name, v: item.value };
            })
        });
    }
}
