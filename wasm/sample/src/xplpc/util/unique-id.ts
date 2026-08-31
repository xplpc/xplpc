export class UniqueID {
    // A number is a double, so incrementing one stops changing it past the safe integer and every key after that is the same one.
    private static uid = 0n;

    static generate(): string {
        return "JS-" + (++UniqueID.uid).toString();
    }
}
