import { Config } from "@/xplpc/core/config";
import { XPLPC } from "@/xplpc/core/xplpc";
import { JsonSerializer } from "@/xplpc/serializer/json-serializer";
import { DataView } from "@/xplpc/type/data-view";

import Module from "@xplpc/build/wasm/wasm32/bin/xplpc";

describe("Heap", () => {
    beforeAll(async () => {
        XPLPC.shared().initialize(
            await Module(),
            new Config(new JsonSerializer()),
        );
    });

    test("reads a view created here after the heap grew", () => {
        // Every read asks the table for the bytes again, so an array detached by the growth is never the one handed to the body.

        const module = XPLPC.shared().module;
        const bufferBefore = module.HEAPU8.buffer;
        const dataView = DataView.createFromArrayBufferView(
            new Uint8Array([1, 2, 3, 4]),
        );
        const detached = DataView.withUint8Array(dataView, (bytes) => bytes);
        const large = DataView.createFromArrayBuffer(
            new ArrayBuffer(64 * 1024 * 1024),
        );

        expect(module.HEAPU8.buffer).not.toBe(bufferBefore);
        expect(detached.length).toBe(0);

        DataView.withUint8Array(dataView, (bytes) => {
            expect(Array.from(bytes)).toEqual([1, 2, 3, 4]);
        });

        DataView.free(large);
        DataView.free(dataView);
    });

    test("copies a view of the heap even when the copy grows the heap", () => {
        // The source is as large as the whole heap, so the allocation made for its copy has to grow the memory the source lives in.

        const module = XPLPC.shared().module;
        const size = module.HEAPU8.length;
        const source = DataView.createFromArrayBuffer(new ArrayBuffer(size));

        DataView.withUint8Array(source, (bytes) => {
            bytes[0] = 7;
            bytes[1] = 8;
            bytes[bytes.length - 1] = 9;
        });

        const bufferBefore = module.HEAPU8.buffer;
        const copy = DataView.withUint8Array(source, (bytes) =>
            DataView.createFromArrayBufferView(bytes),
        );

        expect(module.HEAPU8.buffer).not.toBe(bufferBefore);

        DataView.withUint8Array(copy, (bytes) => {
            expect(bytes.length).toBe(size);
            expect(bytes[0]).toBe(7);
            expect(bytes[1]).toBe(8);
            expect(bytes[bytes.length - 1]).toBe(9);
        });

        DataView.free(copy);
        DataView.free(source);
    });

    test("writes nothing when the allocation fails", () => {
        const module = XPLPC.shared().module;
        const malloc = module._malloc;
        const register = vi.spyOn(module.JavascriptBridge, "registerBuffer");
        const error = vi.spyOn(console, "error").mockImplementation(() => {});
        const before = Array.from(module.HEAPU8.subarray(0, 4));

        module._malloc = () => 0;

        try {
            const dataView = DataView.createFromArrayBuffer(
                new Uint8Array([1, 2, 3, 4]).buffer,
            );

            expect(dataView.handle).toBe(0n);
            expect(register).not.toHaveBeenCalled();
            expect(Array.from(module.HEAPU8.subarray(0, 4))).toEqual(before);
            expect(error).toHaveBeenCalledTimes(1);
        } finally {
            module._malloc = malloc;
            register.mockRestore();
            error.mockRestore();
        }
    });

    test("gives the memory back when the table refuses to register it", () => {
        // A view the table refused names nothing, so it cannot also be the only thing holding the memory that was allocated for it.

        const module = XPLPC.shared().module;
        const register = vi
            .spyOn(module.JavascriptBridge, "registerBuffer")
            .mockImplementationOnce(() => 0n);
        const release = vi.spyOn(module.JavascriptBridge, "releaseBuffer");
        const malloc = vi.spyOn(module, "_malloc");
        const free = vi.spyOn(module, "_free");

        try {
            const dataView = DataView.createFromArrayBuffer(new ArrayBuffer(4));

            expect(dataView.handle).toBe(0n);
            expect(free).toHaveBeenCalledTimes(1);
            expect(free).toHaveBeenCalledWith(malloc.mock.results[0].value);

            DataView.free(dataView);

            expect(free).toHaveBeenCalledTimes(1);
            expect(release).not.toHaveBeenCalled();
        } finally {
            register.mockRestore();
            release.mockRestore();
            malloc.mockRestore();
            free.mockRestore();
        }
    });

    test("allocates nothing for an empty source", () => {
        const module = XPLPC.shared().module;
        const malloc = vi.spyOn(module, "_malloc");
        const register = vi.spyOn(module.JavascriptBridge, "registerBuffer");

        try {
            const dataView = DataView.createFromArrayBuffer(new ArrayBuffer(0));

            expect(dataView.handle).toBe(0n);
            expect(malloc).not.toHaveBeenCalled();
            expect(register).not.toHaveBeenCalled();

            DataView.withUint8Array(dataView, (bytes) => {
                expect(bytes.length).toBe(0);
            });
        } finally {
            malloc.mockRestore();
            register.mockRestore();
        }
    });
});
