import { XPLPC } from "../core/xplpc";
import { Log } from "../util/log";

// Names a buffer registered by whoever owns the bytes, which is all that crosses the wire, so a view can never point a reader at memory nobody registered.
export class DataView {
    private buffer: bigint;
    private address = 0;

    constructor(handle = 0n) {
        this.buffer = handle;
    }

    get handle(): bigint {
        return this.buffer;
    }

    toJSON(): { buffer: number } {
        // The table never answers a handle a javascript number cannot hold, so only a view built from some other handle can need more precision.
        if (this.buffer > BigInt(Number.MAX_SAFE_INTEGER)) {
            throw new RangeError(
                "The buffer handle needs more precision than a javascript number holds",
            );
        }

        return { buffer: Number(this.buffer) };
    }

    // Turns what the wire carried into a view, and anything that is not a handle names no buffer.
    static fromJSON(value: unknown): DataView {
        const buffer =
            typeof value === "object" && value !== null && "buffer" in value
                ? value.buffer
                : undefined;

        // The parser already refused a whole number past the safe range, so a number that passes here is the exact handle.
        if (
            typeof buffer !== "number" ||
            !Number.isSafeInteger(buffer) ||
            buffer < 0
        ) {
            Log.e("[DataView : fromJSON] Value is not a data view");
            Log.d(
                "[DataView : fromJSON] Value is not a data view: " +
                    typeof buffer,
            );

            return new DataView();
        }

        return new DataView(BigInt(buffer));
    }

    // Copies the bytes into the heap and registers them, and the view owns both until it is released with free.
    static createFromArrayBuffer(buffer: ArrayBufferLike): DataView {
        return DataView.copyToHeap(new Uint8Array(buffer));
    }

    static createFromArrayBufferView(view: ArrayBufferView): DataView {
        return DataView.copyToHeap(
            new Uint8Array(view.buffer, view.byteOffset, view.byteLength),
        );
    }

    // The bytes are handed to the body rather than returned, because growing the heap detaches the array they are read through.
    static withUint8Array<T>(
        dataView: DataView,
        body: (bytes: Uint8Array<ArrayBuffer>) => T,
    ): T {
        return body(DataView.read(dataView));
    }

    static withUint8ClampedArray<T>(
        dataView: DataView,
        body: (bytes: Uint8ClampedArray<ArrayBuffer>) => T,
    ): T {
        const bytes = DataView.read(dataView);

        return body(
            new Uint8ClampedArray(
                bytes.buffer,
                bytes.byteOffset,
                bytes.byteLength,
            ),
        );
    }

    // Releases the registration and the heap memory of a view created here, and ignores a view it did not allocate.
    static free(dataView: DataView): void {
        if (dataView.address === 0) {
            return;
        }

        const module = XPLPC.shared().module;

        // The registration goes first, so the table never names memory that was already given back.
        module.JavascriptBridge.releaseBuffer(dataView.buffer);
        module._free(dataView.address);

        dataView.buffer = 0n;
        dataView.address = 0;
    }

    private static read(dataView: DataView): Uint8Array<ArrayBuffer> {
        // A view naming nothing is the empty view every refusal answers with, so it is not looked up.
        if (dataView.buffer === 0n) {
            return new Uint8Array(0);
        }

        if (!XPLPC.shared().initialized) {
            Log.e("[DataView : read] XPLPC was not initialized");
            return new Uint8Array(0);
        }

        const bytes = XPLPC.shared().module.JavascriptBridge.readBuffer(
            dataView.buffer,
        );

        return bytes ?? new Uint8Array(0);
    }

    private static copyToHeap(view: Uint8Array): DataView {
        if (view.byteLength === 0) {
            return new DataView();
        }

        if (!XPLPC.shared().initialized) {
            Log.e("[DataView : copyToHeap] XPLPC was not initialized");
            return new DataView();
        }

        const module = XPLPC.shared().module;

        // Allocating can grow the heap, which detaches every array taken from it, so a source living there is copied out first.
        const source =
            view.buffer === module.HEAPU8.buffer ? view.slice() : view;
        const address = module._malloc(source.byteLength);

        if (address === 0) {
            Log.e(
                "[DataView : copyToHeap] There was no memory to copy the bytes into",
            );
            return new DataView();
        }

        module.HEAPU8.set(source, address);

        const handle = module.JavascriptBridge.registerBuffer(
            address,
            source.byteLength,
        );

        if (handle === 0n) {
            module._free(address);
            return new DataView();
        }

        const dataView = new DataView(handle);
        dataView.address = address;

        return dataView;
    }
}
