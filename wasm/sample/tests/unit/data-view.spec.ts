import { Config } from "@/xplpc/core/config";
import { XPLPC } from "@/xplpc/core/xplpc";
import { JsonSerializer } from "@/xplpc/serializer/json-serializer";
import { DataView } from "@/xplpc/type/data-view";

import Module from "@xplpc/build/wasm/wasm32/bin/xplpc";

// What the native side wrote, which is where a read the table refused is reported.
const nativeLines: string[] = [];

function bytesOf(dataView: DataView): number[] {
    return DataView.withUint8Array(dataView, (bytes) => Array.from(bytes));
}

function nativeErrors(): string[] {
    return nativeLines.filter((line) => line.includes("[error]"));
}

describe("DataView", () => {
    beforeAll(async () => {
        XPLPC.shared().initialize(
            await Module({ print: (line) => nativeLines.push(line) }),
            new Config(new JsonSerializer()),
        );
    });

    beforeEach(() => {
        nativeLines.length = 0;
    });

    afterEach(() => {
        expect(XPLPC.shared().module.JavascriptBridge.pendingCount()).toBe(0);
    });

    test("puts only its handle on the wire", () => {
        const dataView = DataView.createFromArrayBuffer(new ArrayBuffer(8));

        expect(dataView.handle).not.toBe(0n);
        expect(JSON.stringify(dataView)).toBe(
            '{"buffer":' + dataView.handle.toString() + "}",
        );

        DataView.free(dataView);
    });

    test("reads back the bytes it was created from", () => {
        const dataView = DataView.createFromArrayBufferView(
            new Uint8Array([1, 2, 3, 250]),
        );

        expect(bytesOf(dataView)).toEqual([1, 2, 3, 250]);

        DataView.withUint8ClampedArray(dataView, (bytes) => {
            expect(Array.from(bytes)).toEqual([1, 2, 3, 250]);
        });

        DataView.free(dataView);
    });

    test("decodes a handle the wire carried", () => {
        const dataView = DataView.createFromArrayBufferView(
            new Uint8Array([4, 5, 6]),
        );
        const decoded = DataView.fromJSON(JSON.parse(JSON.stringify(dataView)));

        expect(decoded.handle).toBe(dataView.handle);
        expect(bytesOf(decoded)).toEqual([4, 5, 6]);

        DataView.free(dataView);
    });

    test("decodes anything but a handle as a view naming nothing", () => {
        // A boolean, a negative number, a fraction and text are what a reader would otherwise turn into a handle it never received.

        const refused: unknown[] = [
            { buffer: true },
            { buffer: -1 },
            { buffer: 1.5 },
            { buffer: "1" },
            { buffer: null },
            { buffer: Number.MAX_SAFE_INTEGER + 2 },
            { ptr: 128, size: 4 },
            {},
            null,
            undefined,
            7,
            "buffer",
        ];

        const error = vi.spyOn(console, "error").mockImplementation(() => {});

        try {
            for (const value of refused) {
                const dataView = DataView.fromJSON(value);

                expect(dataView.handle).toBe(0n);
                expect(bytesOf(dataView)).toEqual([]);
            }

            // A view naming nothing is never looked up, so the table reports nothing for it.
            expect(error).toHaveBeenCalledTimes(refused.length);
            expect(nativeErrors()).toEqual([]);
        } finally {
            error.mockRestore();
        }
    });

    test("reads nothing once the view was released", () => {
        // A second view naming the same handle is what a mapping still holding the old one would read through.

        const dataView = DataView.createFromArrayBufferView(
            new Uint8Array([9, 9, 9, 9]),
        );
        const kept = new DataView(dataView.handle);

        DataView.free(dataView);

        const error = vi.spyOn(console, "error").mockImplementation(() => {});

        try {
            expect(dataView.handle).toBe(0n);
            expect(bytesOf(dataView)).toEqual([]);
            expect(bytesOf(kept)).toEqual([]);
            expect(nativeErrors()).toHaveLength(1);
            expect(nativeErrors()[0]).toContain("[BufferTable : read]");
        } finally {
            error.mockRestore();
        }
    });

    test("a stale handle does not read the buffer that took its slot", () => {
        const first = DataView.createFromArrayBufferView(
            new Uint8Array([1, 1, 1, 1]),
        );
        const stale = new DataView(first.handle);

        DataView.free(first);

        const second = DataView.createFromArrayBufferView(
            new Uint8Array([2, 2, 2, 2]),
        );

        // The slot is reused and its generation moves on, which is what makes the old handle stale rather than a name for the new bytes.
        expect(second.handle >> 32n).toBe(stale.handle >> 32n);
        expect(second.handle).not.toBe(stale.handle);

        const error = vi.spyOn(console, "error").mockImplementation(() => {});

        try {
            expect(bytesOf(stale)).toEqual([]);
            expect(nativeErrors()).toHaveLength(1);
        } finally {
            error.mockRestore();
        }

        expect(bytesOf(second)).toEqual([2, 2, 2, 2]);

        DataView.free(second);
    });

    test("reads nothing from a handle that was never registered", () => {
        const error = vi.spyOn(console, "error").mockImplementation(() => {});

        try {
            expect(bytesOf(new DataView((1000n << 32n) | 1n))).toEqual([]);
            expect(nativeErrors()).toHaveLength(1);
        } finally {
            error.mockRestore();
        }
    });

    test("free ignores a view it did not allocate", () => {
        // Only the layer that registered a buffer may release it, so a view naming someone else's handle releases nothing.

        const dataView = DataView.createFromArrayBufferView(
            new Uint8Array([3, 3]),
        );
        const named = new DataView(dataView.handle);

        DataView.free(named);

        expect(named.handle).toBe(dataView.handle);
        expect(bytesOf(dataView)).toEqual([3, 3]);

        DataView.free(dataView);
    });

    test("free releases a view once", () => {
        const module = XPLPC.shared().module;
        const release = vi.spyOn(module.JavascriptBridge, "releaseBuffer");
        const free = vi.spyOn(module, "_free");

        try {
            const dataView = DataView.createFromArrayBuffer(new ArrayBuffer(4));

            DataView.free(dataView);
            DataView.free(dataView);

            expect(release).toHaveBeenCalledTimes(1);
            expect(release).toHaveReturnedWith(true);
            expect(free).toHaveBeenCalledTimes(1);
            expect(nativeErrors()).toEqual([]);
        } finally {
            release.mockRestore();
            free.mockRestore();
        }
    });
});
