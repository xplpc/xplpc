import { XPLPC } from "../core/xplpc";
import { Param } from "./param";

export class Request {
    functionName: string;
    params: Param[];

    constructor(functionName: string, ...params: Param[]) {
        this.functionName = functionName;
        this.params = params;
    }

    data(): string {
        return XPLPC.shared().config.serializer.encodeRequest(
            this.functionName,
            ...this.params,
        );
    }
}
