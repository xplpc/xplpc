import { XPLPC } from "../core/xplpc";

export class DataView {
    ptr: number;
    size: number;

    private owned = false;

    constructor(ptr: number, size: number) {
        this.ptr = ptr;
        this.size = size;
    }

    toJSON(): { ptr: number; size: number } {
        // Only the address and the length travel, any other field is local bookkeeping.

        return { ptr: this.ptr, size: this.size };
    }

    static createFromArrayBuffer(buffer: ArrayBufferLike): DataView {
        return DataView.copyToHeap(new Uint8Array(buffer));
    }

    static createFromArrayBufferView(view: ArrayBufferView): DataView {
        return DataView.copyToHeap(
            new Uint8Array(view.buffer, view.byteOffset, view.byteLength),
        );
    }

    static withUint8Array<T>(
        dataView: DataView,
        body: (bytes: Uint8Array<ArrayBuffer>) => T,
    ): T {
        // The bytes are handed to the body rather than returned, because growing the heap replaces the buffer every view was taken from.

        if (!DataView.describesBytes(dataView)) {
            return body(new Uint8Array(0));
        }

        return body(
            new Uint8Array(
                XPLPC.shared().module.HEAPU8.buffer,
                dataView.ptr,
                dataView.size,
            ),
        );
    }

    static withUint8ClampedArray<T>(
        dataView: DataView,
        body: (bytes: Uint8ClampedArray<ArrayBuffer>) => T,
    ): T {
        if (!DataView.describesBytes(dataView)) {
            return body(new Uint8ClampedArray(0));
        }

        return body(
            new Uint8ClampedArray(
                XPLPC.shared().module.HEAPU8.buffer,
                dataView.ptr,
                dataView.size,
            ),
        );
    }

    private static describesBytes(dataView: DataView): boolean {
        // A view that carries no address describes nothing to read, whatever length it claims.

        return dataView.ptr > 0 && dataView.size > 0;
    }

    static free(dataView: DataView): void {
        if (!dataView.owned) {
            return;
        }

        XPLPC.shared().module._free(dataView.ptr);

        dataView.owned = false;
        dataView.ptr = 0;
        dataView.size = 0;
    }

    private static copyToHeap(view: Uint8Array): DataView {
        // The returned view owns heap memory, which is released with free.

        const module = XPLPC.shared().module;
        const ptr = module._malloc(view.byteLength);

        module.HEAPU8.set(view, ptr);

        const dataView = new DataView(ptr, view.byteLength);
        dataView.owned = true;

        return dataView;
    }
}
