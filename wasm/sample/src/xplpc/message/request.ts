import { XPLPC } from "../core/xplpc";
import { XParam } from "./param";

export class XRequest {
    functionName: string;
    params: XParam[];

    constructor(functionName: string, ...params: XParam[]) {
        this.functionName = functionName;
        this.params = params;
    }

    data(): string {
        return XPLPC.shared().config.serializer.encodeRequest(this.functionName, ...this.params);
    }
}
