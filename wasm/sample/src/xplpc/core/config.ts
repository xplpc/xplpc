import { IXSerializer } from "../serializer/serializer";

export class XConfig {
    public serializer: IXSerializer

    constructor(serializer: IXSerializer) {
        this.serializer = serializer
    }
}
