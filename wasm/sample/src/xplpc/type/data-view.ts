import { XPLPC } from "../core/xplpc";

export class XDataView {
    ptr: number;
    size: number;

    constructor(ptr: number, size: number) {
        this.ptr = ptr;
        this.size = size;
    }

    static createFromArrayBuffer(data: ArrayBufferLike): XDataView {
        const size = data.byteLength;

        // eslint-disable-next-line
        // @ts-ignore:next-line
        const ptr = XPLPC.shared().module._malloc(size);

        // eslint-disable-next-line
        // @ts-ignore:next-line
        XPLPC.shared().module.HEAPU8.set(data, ptr);

        return new XDataView(ptr, size);
    }

    static createUint8ArrayFromPtr(ptr: number, size: number): Uint8Array {
        // eslint-disable-next-line
        // @ts-ignore:next-line
        return new Uint8Array(XPLPC.shared().module.HEAPU8.buffer, ptr, size);
    }

    static createUint8ClampedArrayFromPtr(ptr: number, size: number): Uint8ClampedArray {
        // eslint-disable-next-line
        // @ts-ignore:next-line
        return new Uint8ClampedArray(XPLPC.shared().module.HEAPU8.buffer, ptr, size);
    }

    static free(dataView: XDataView): void {
        // eslint-disable-next-line
        // @ts-ignore:next-line
        XPLPC.shared().module._free(dataView.ptr);
    }
}
