import { Serializer } from "../serializer/serializer";

export class Config {
    public readonly serializer: Serializer;

    constructor(serializer: Serializer) {
        this.serializer = serializer;
    }
}
