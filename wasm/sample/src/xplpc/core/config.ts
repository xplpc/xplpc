import { Serializer } from "../serializer/serializer";

export class Config {
    public serializer: Serializer;

    constructor(serializer: Serializer) {
        this.serializer = serializer;
    }
}
