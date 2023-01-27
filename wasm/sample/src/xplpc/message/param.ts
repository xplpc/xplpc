export class XParam {
    name: string;
    value?: unknown;

    constructor(name: string, value?: unknown) {
        this.name = name;
        this.value = value;
    }
}
