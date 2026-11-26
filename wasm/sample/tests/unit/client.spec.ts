import { Callback } from "@/custom/callback";
import { Client } from "@/xplpc/client/client";
import { Config } from "@/xplpc/core/config";
import { XPLPC } from "@/xplpc/core/xplpc";
import { MappingList } from "@/xplpc/data/mapping-list";
import { Layer } from "@/xplpc/layer/layer";
import { MappingItem } from "@/xplpc/map/mapping-item";
import { DecodedRequest } from "@/xplpc/message/decoded-request";
import { Param } from "@/xplpc/message/param";
import { Request } from "@/xplpc/message/request";
import { WasmModule } from "@/xplpc/module/xplpc-module";
import { JsonSerializer } from "@/xplpc/serializer/json-serializer";
import { DataView } from "@/xplpc/type/data-view";

import Module from "@xplpc/build/wasm/wasm32/bin/xplpc";

function deferredAnswer(): Promise<string> {
    return new Promise<string>((resolve) => {
        setTimeout(() => resolve("deferred"), 50);
    });
}

function raisingBeforeReturning(): Promise<string> {
    // The mapping raises before it can return a promise, which is the shape a rejection never covers.
    throw new Error("the mapping raised");
}

function rejectingAfterReturning(): Promise<string> {
    return Promise.reject(new Error("the mapping rejected"));
}

// A red, a green, a blue and a transparent pixel, four bytes each.
function fourPixels(): Uint8Array {
    return new Uint8Array([
        255, 0, 0, 255, 0, 255, 0, 255, 0, 0, 255, 255, 0, 0, 0, 0,
    ]);
}

function hostLayer(): Layer {
    return new Layer(XPLPC.shared().module, XPLPC.shared().config);
}

// Replaces the answer the layer hands back to the router, so a test can invoke the layer directly and read what it answered.
function recordAnswers(
    body: () => void | Promise<void>,
): Promise<[bigint, string][]> {
    const bridge = XPLPC.shared().module.JavascriptBridge;
    const original = bridge.answer;
    const answers: [bigint, string][] = [];

    bridge.answer = (call: bigint, data: string) => {
        answers.push([call, data]);
    };

    return Promise.resolve()
        .then(body)
        .then(() => answers)
        .finally(() => {
            bridge.answer = original;
        });
}

function reverseResponse(): Promise<string> {
    return new Promise<string>((resolve) => {
        return resolve("ok");
    });
}

describe("Client", () => {
    afterEach(() => {
        // A leaked call is the defect this project has found most often, and the router is the one registry left, so every test is held to leaving nothing pending in it.
        expect(XPLPC.shared().module.JavascriptBridge.pendingCount()).toBe(0);
    });

    beforeAll(async () => {
        const module: WasmModule = await Module();
        XPLPC.shared().initialize(module, new Config(new JsonSerializer()));

        MappingList.shared().add(
            "platform.battery.level",
            new MappingItem(Callback.batteryLevel),
        );
        MappingList.shared().add(
            "platform.reverse.response",
            new MappingItem(reverseResponse),
        );
        MappingList.shared().add(
            "platform.deferred.answer",
            new MappingItem(deferredAnswer),
        );
    });

    test("battery level", () => {
        const request = new Request(
            "platform.battery.level",
            new Param("suffix", "%"),
        );

        return Client.call<string>(request).then(
            (response: string | undefined) => {
                expect(response).toBe("100%");
            },
        );
    });

    test("battery level without a suffix answers the level alone", async () => {
        // A parameter may arrive without a value, so the sample mapping reads an absent suffix as none rather than writing the word for it.

        expect(
            await Client.call<string>(new Request("platform.battery.level")),
        ).toBe("100");
        expect(
            await Client.call<string>(
                new Request("platform.battery.level", new Param("suffix")),
            ),
        ).toBe("100");
    });

    test("battery level from string", async () => {
        const request = new Request(
            "platform.battery.level",
            new Param("suffix", "%"),
        );
        const response = await Client.call(request.data());
        expect(response).toBe('{"r":"100%"}');
    });

    it("get logged value", async () => {
        const request = new Request(
            "sample.login",
            new Param("username", "paulo"),
            new Param("password", "123456"),
            new Param("remember", true),
        );

        const response = await Client.call<string>(request);
        expect(response).toBe("LOGGED-WITH-REMEMBER");
    });

    it("get reverse value", async () => {
        const request = new Request("sample.reverse");

        const response = await Client.call<string>(request);
        expect(response).toBe("response-is-ok");
    });

    test("transfer data view", async () => {
        const dataView = DataView.createFromArrayBufferView(fourPixels());

        const request = new Request(
            "sample.image.grayscale.dataview",
            new Param("dataView", dataView),
        );

        const response = await Client.call<string>(request);

        expect("OK").toBe(response);

        DataView.withUint8Array(dataView, (processedData) => {
            expect(16).toBe(processedData.length);
            expect(processedData[0]).toBe(85);
            expect(processedData[4]).toBe(85);
            expect(processedData[8]).toBe(85);
            expect(processedData[12]).toBe(0);
        });

        DataView.withUint8ClampedArray(dataView, (processedData) => {
            expect(processedData).toBeInstanceOf(Uint8ClampedArray);
            expect(processedData[0]).toBe(85);
        });

        DataView.free(dataView);
    });

    test("receive data view", async () => {
        const request = new Request("sample.dataview");
        const dataView = DataView.fromJSON(await Client.call<unknown>(request));

        expect(dataView.handle).not.toBe(0n);

        DataView.withUint8Array(dataView, (originalData) => {
            expect(16).toBe(originalData.length);
            expect(originalData[0]).toBe(255);
            expect(originalData[5]).toBe(255);
            expect(originalData[10]).toBe(255);
            expect(originalData[12]).toBe(0);
        });

        const request2 = new Request(
            "sample.image.grayscale.dataview",
            new Param("dataView", dataView),
        );

        const response2 = await Client.call<string>(request2);

        expect("OK").toBe(response2);

        // The mapping wrote into the same block, so reading it again shows the processed bytes.
        DataView.withUint8Array(dataView, (processedData) => {
            expect(16).toBe(processedData.length);
            expect(processedData[0]).toBe(85);
            expect(processedData[4]).toBe(85);
            expect(processedData[8]).toBe(85);
            expect(processedData[12]).toBe(0);
        });
    });

    test("read a data view after the heap grew", async () => {
        // Growing the heap replaces the buffer, so a read taken from a kept view would land on memory that is gone.

        const dataView = DataView.fromJSON(
            await Client.call<unknown>(new Request("sample.dataview")),
        );

        expect(dataView.handle).not.toBe(0n);

        const bufferBefore = XPLPC.shared().module.HEAPU8.buffer;
        const large = DataView.createFromArrayBuffer(
            new ArrayBuffer(64 * 1024 * 1024),
        );

        expect(XPLPC.shared().module.HEAPU8.buffer).not.toBe(bufferBefore);

        DataView.withUint8Array(dataView, (bytes) => {
            expect(16).toBe(bytes.length);
            expect(bytes[0]).toBe(255);
            expect(bytes[5]).toBe(255);
            expect(bytes[10]).toBe(255);
            expect(bytes[12]).toBe(0);
        });

        DataView.free(large);
    });

    test("write into the heap and let cxx work on the same bytes", async () => {
        // The whole point of a data view is that neither side copies, so a write here has to be the bytes cxx reads.

        const dataView = DataView.createFromArrayBuffer(new ArrayBuffer(16));

        DataView.withUint8Array(dataView, (bytes) => {
            bytes.set([
                10, 20, 60, 255, 10, 20, 60, 255, 10, 20, 60, 255, 0, 0, 0, 0,
            ]);
        });

        // A later read sees the write, so nothing was copied on the way in.
        DataView.withUint8Array(dataView, (bytes) => {
            expect(bytes[0]).toBe(10);
            expect(bytes[1]).toBe(20);
            expect(bytes[2]).toBe(60);
        });

        const response = await Client.call<string>(
            new Request(
                "sample.image.grayscale.dataview",
                new Param("dataView", dataView),
            ),
        );

        expect("OK").toBe(response);

        // Cxx averaged the bytes written here and wrote the result into the same block.
        DataView.withUint8Array(dataView, (bytes) => {
            expect(bytes[0]).toBe(30);
            expect(bytes[1]).toBe(30);
            expect(bytes[2]).toBe(30);
            expect(bytes[3]).toBe(255);
            expect(bytes[12]).toBe(0);
        });

        DataView.free(dataView);
    });

    test("free ignores a data view it did not allocate", async () => {
        // The view names a buffer the c++ side registered, which only that side may release.

        const borrowed = DataView.fromJSON(
            await Client.call<unknown>(new Request("sample.dataview")),
        );
        const handle = borrowed.handle;

        expect(handle).not.toBe(0n);

        DataView.free(borrowed);

        expect(borrowed.handle).toBe(handle);

        DataView.withUint8Array(borrowed, (bytes) => {
            expect(bytes.length).toBe(16);
        });
    });

    test("a view released before the call reaches cxx as nothing", async () => {
        // Cxx reads through the table, so a handle the page released names no bytes even though the heap still holds them.

        const dataView = DataView.createFromArrayBuffer(new ArrayBuffer(16));
        const released = new DataView(dataView.handle);

        DataView.free(dataView);

        const error = vi.spyOn(console, "error").mockImplementation(() => {});

        try {
            const response = await Client.call<string>(
                new Request(
                    "sample.image.grayscale.dataview",
                    new Param("dataView", released),
                ),
            );

            expect(response).toBe("INVALID-DATA");
        } finally {
            error.mockRestore();
        }
    });

    test("cxx reads nothing from a handle that is not an unsigned integer", async () => {
        // A boolean and a negative number are what the format refuses where a handle belongs, so cxx reads no buffer from either.

        for (const buffer of ["true", "-1", "1.5", '"1"']) {
            const response = await Client.call(
                '{"f":"sample.image.grayscale.dataview","p":[{"n":"dataView","v":{"buffer":' +
                    buffer +
                    "}}]}",
            );

            expect(response).toBe('{"r":"INVALID-DATA"}');
        }
    });

    test("todo round trip", async () => {
        // An object crossing the bridge exercises the nested shapes the primitives never reach.

        const response = await Client.call<Record<string, unknown>>(
            new Request(
                "sample.todo.single",
                new Param("item", {
                    id: 1,
                    title: "Title 1",
                    body: "Body 1",
                    data: { data1: "value1" },
                    done: true,
                }),
            ),
        );

        expect(response?.id).toBe(1);
        expect(response?.title).toBe("Title 1");
        expect((response?.data as Record<string, string>).data1).toBe("value1");
        expect(response?.done).toBe(true);
    });

    test("echoes every encoding width", async () => {
        // A string has to survive every width utf8 can encode it in, on the way out and on the way back.

        const samples = [
            "plain ascii",
            "caf\u00e9 na\u00efve",
            "\u4e2d\u6587\u30c6\u30b9\u30c8",
            "\u{1F600}\u{1F468}\u200d\u{1F469}",
            "mixed \u00e9 \u4e2d \u{1F600} end",
        ];

        for (const sample of samples) {
            const response = await Client.call<string>(
                new Request("sample.echo", new Param("value", sample)),
            );

            expect(response).toBe(sample);
        }
    });

    test("carries a large data view without truncating", async () => {
        // A buffer far past what a small integer holds has to cross without being truncated anywhere on the way.

        const size = 4 * 1024 * 1024;
        const dataView = DataView.createFromArrayBuffer(
            new Uint8Array(size).fill(100).buffer,
        );

        const response = await Client.call<string>(
            new Request(
                "sample.image.grayscale.dataview",
                new Param("dataView", dataView),
            ),
        );

        expect(response).toBe("OK");

        DataView.withUint8Array(dataView, (bytes) => {
            expect(bytes.length).toBe(size);
            expect(bytes[0]).toBe(100);
            expect(bytes[size - 1]).toBe(100);
        });

        DataView.free(dataView);
    });

    // The edges are what a serializer loses, since a small value round trips even when the format cannot carry the type.
    function allTypesItem(): Record<string, unknown> {
        return {
            typeInt8: -128,
            typeInt16: -32768,
            typeInt32: -2147483648,
            // A javascript number is a double, so the largest integer this bridge carries exactly is the safe one.
            typeInt64: Number.MAX_SAFE_INTEGER,
            typeFloat32: 0.1,
            typeFloat64: 0.1,
            typeBool: true,
            typeOptional: null,
            typeList: [],
            typeMap: { item1: "ok" },
            typeDateTime: 494938800,
            typeChar: 122,
            typeChar16: 174,
            typeChar32: 174,
            typeWchar: 174,
            typeString: "ok",
        };
    }

    test("keeps every edge of an all types item", async () => {
        const item = allTypesItem();
        item.typeList = [allTypesItem()];

        const response = await Client.call<Record<string, unknown>>(
            new Request("sample.alltypes.single", new Param("item", item)),
        );

        expect(response?.typeInt8).toBe(-128);
        expect(response?.typeInt16).toBe(-32768);
        expect(response?.typeInt32).toBe(-2147483648);
        expect(response?.typeInt64).toBe(Number.MAX_SAFE_INTEGER);
        expect(Math.abs((response?.typeFloat32 as number) - 0.1)).toBeLessThan(
            1e-7,
        );
        expect(response?.typeFloat64).toBe(0.1);
        expect(response?.typeBool).toBe(true);
        expect(response?.typeOptional).toBe(null);
        expect((response?.typeList as unknown[]).length).toBe(1);
        expect((response?.typeMap as Record<string, string>).item1).toBe("ok");
        expect(response?.typeDateTime).toBe(494938800);
        expect(response?.typeChar).toBe(122);
        expect(response?.typeChar16).toBe(174);
        expect(response?.typeChar32).toBe(174);
        expect(response?.typeWchar).toBe(174);
        expect(response?.typeString).toBe("ok");
    });

    test("carries a list of all types items", async () => {
        const response = await Client.call<Record<string, unknown>[]>(
            new Request(
                "sample.alltypes.list",
                new Param("items", [allTypesItem(), allTypesItem()]),
            ),
        );

        expect(response?.length).toBe(2);
        expect(response?.[0].typeInt64).toBe(Number.MAX_SAFE_INTEGER);
        expect(response?.[1].typeString).toBe("ok");
    });

    test("carries a list of todo items", async () => {
        const response = await Client.call<Record<string, unknown>[]>(
            new Request(
                "sample.todo.list",
                new Param("items", [
                    {
                        id: 1,
                        title: "Title 1",
                        body: "Body 1",
                        data: {},
                        done: true,
                    },
                    {
                        id: 2,
                        title: "Title 2",
                        body: "Body 2",
                        data: {},
                        done: true,
                    },
                ]),
            ),
        );

        expect(response?.length).toBe(2);
        expect(response?.[0].title).toBe("Title 1");
        expect(response?.[1].title).toBe("Title 2");
    });

    test("answers empty when a mapping needs a thread this bridge has none of", async () => {
        // This module is linked without pthreads, so a mapping that starts a thread cannot run here and the caller is answered rather than left waiting.

        const response = await Client.call<string>(new Request("sample.async"));

        expect(response).toBeUndefined();
    });

    test("a call for a name the page declared and holds no mapping for is answered", async () => {
        // The router routes by what the layer declared, so a name declared natively and missing from the page still has to be answered rather than left waiting.

        const bridge = XPLPC.shared().module.JavascriptBridge;
        const error = vi.spyOn(console, "error").mockImplementation(() => {});

        bridge.declare("host.name.nothing.owns");

        try {
            const response = await Client.call<string>(
                new Request("host.name.nothing.owns"),
            );

            expect(response).toBeUndefined();
            expect(error).toHaveBeenCalledWith(
                "[XPLPC]",
                expect.stringContaining("Mapping not found"),
            );
        } finally {
            bridge.retract("host.name.nothing.owns");
            error.mockRestore();
        }
    });

    test("a mapping whose promise rejects answers the handle it was given", async () => {
        MappingList.shared().add(
            "platform.rejecting",
            new MappingItem(rejectingAfterReturning),
        );

        const error = vi.spyOn(console, "error").mockImplementation(() => {});

        const answers = await recordAnswers(async () => {
            hostLayer().invoke(9n, '{"f":"platform.rejecting","p":[]}');

            await new Promise((resolve) => setTimeout(resolve, 0));
        });

        error.mockRestore();

        expect(answers).toEqual([[9n, ""]]);
    });

    test("host mapping that raises before returning still answers the caller", async () => {
        MappingList.shared().add(
            "platform.raising",
            new MappingItem(raisingBeforeReturning),
        );

        const response = await Client.call<string>(
            new Request("platform.raising"),
        );

        expect(response).toBeUndefined();
        expect(XPLPC.shared().module.JavascriptBridge.pendingCount()).toBe(0);
    });

    test("host mapping that rejects answers the native caller and leaves nothing pending", async () => {
        MappingList.shared().add(
            "platform.rejecting",
            new MappingItem(rejectingAfterReturning),
        );

        const response = await Client.call<string>(
            new Request("platform.rejecting"),
        );

        expect(response).toBeUndefined();
        expect(XPLPC.shared().module.JavascriptBridge.pendingCount()).toBe(0);
    });

    test("nested host mapping answering later reaches the outer caller", async () => {
        let resolveInner: ((value: string) => void) | undefined;
        let answered = false;

        MappingList.shared().add(
            "platform.reverse.response",
            new MappingItem(
                () =>
                    new Promise<string>((resolve) => {
                        resolveInner = resolve;
                    }),
            ),
        );

        const pending = Client.call<string>(new Request("sample.reverse")).then(
            (response) => {
                answered = true;
                return response;
            },
        );

        await new Promise((resolve) => setTimeout(resolve, 0));

        expect(answered).toBe(false);
        expect(resolveInner).toBeDefined();

        resolveInner?.("ok");

        expect(await pending).toBe("response-is-ok");
    });

    test("host mapping answers after the call returned", async () => {
        let answered = false;

        const pending = Client.call<string>(
            new Request("platform.deferred.answer"),
        ).then((response) => {
            answered = true;
            return response;
        });

        expect(answered).toBe(false);
        expect(XPLPC.shared().module.JavascriptBridge.pendingCount()).toBe(1);
        expect(await pending).toBe("deferred");
    });

    test("answers empty for a function nothing owns and leaves nothing pending", async () => {
        const error = vi.spyOn(console, "error").mockImplementation(() => {});

        try {
            const response = await Client.call<string>(
                new Request("not.found"),
            );

            expect(response).toBeUndefined();
            expect(XPLPC.shared().module.JavascriptBridge.pendingCount()).toBe(
                0,
            );
        } finally {
            error.mockRestore();
        }
    });

    test("answers empty when the library was not initialized", async () => {
        // A caller reaching out before the library is up is answered rather than raised at, which is the direction this one covers.

        const shared = XPLPC.shared();
        shared.initialized = false;

        try {
            const response = await Client.call<string>(
                new Request("sample.login"),
            );

            expect(response).toBeUndefined();
            expect(await Client.call('{"f":"sample.login","p":[]}')).toBe("");
        } finally {
            shared.initialized = true;
        }
    });

    test("call sync answers the value from an inline native mapping", () => {
        // A mapping the module owns answers before it returns, so the value is there to be read.

        const request = new Request(
            "sample.login",
            new Param("username", "paulo"),
            new Param("password", "123456"),
            new Param("remember", true),
        );

        expect(Client.callSync<string>(request)).toBe("LOGGED-WITH-REMEMBER");
    });

    test("call sync answers the document from a string", () => {
        const request = new Request(
            "sample.login",
            new Param("username", "paulo"),
            new Param("password", "123456"),
            new Param("remember", true),
        );

        expect(Client.callSync(request.data())).toBe(
            '{"r":"LOGGED-WITH-REMEMBER"}',
        );
    });

    test("call sync answers empty for a page mapping and leaves nothing pending", async () => {
        // A mapping this bridge owns answers with a promise, which resolves in a microtask and never inside the call.

        const bridge = XPLPC.shared().module.JavascriptBridge;
        const error = vi.spyOn(console, "error").mockImplementation(() => {});

        try {
            const request = new Request(
                "platform.battery.level",
                new Param("suffix", "%"),
            );

            expect(Client.callSync<string>(request)).toBeUndefined();
            expect(bridge.pendingCount()).toBe(0);
            expect(Client.callSync(request.data())).toBe("");
            expect(bridge.pendingCount()).toBe(0);
            expect(error).toHaveBeenCalledWith(
                "[XPLPC]",
                expect.stringContaining("did not answer synchronously"),
            );

            // The answer the mapping produces a microtask later reaches a cancelled call, which is a no-op that reports nothing.
            error.mockClear();
            await new Promise((resolve) => setTimeout(resolve, 0));

            expect(bridge.pendingCount()).toBe(0);
            expect(error).not.toHaveBeenCalled();
        } finally {
            error.mockRestore();
        }
    });

    test("call sync answers empty for a function nothing owns and leaves nothing pending", () => {
        const error = vi.spyOn(console, "error").mockImplementation(() => {});

        try {
            expect(
                Client.callSync<string>(new Request("not.found")),
            ).toBeUndefined();
            expect(Client.callSync('{"f":"not.found","p":[]}')).toBe("");
            expect(XPLPC.shared().module.JavascriptBridge.pendingCount()).toBe(
                0,
            );
        } finally {
            error.mockRestore();
        }
    });

    test("call sync answers empty when the library was not initialized", () => {
        const shared = XPLPC.shared();
        const error = vi.spyOn(console, "error").mockImplementation(() => {});

        shared.initialized = false;

        try {
            expect(
                Client.callSync<string>(new Request("sample.login")),
            ).toBeUndefined();
        } finally {
            error.mockRestore();
            shared.initialized = true;
        }
    });

    test("answers the caller when a serializer raises", async () => {
        // A serializer is code this library may not have written, whether it fails while reading the request or while writing the answer.

        class RaisingDecoder extends JsonSerializer {
            override decodeRequest(): DecodedRequest | undefined {
                throw new Error("the request could not be read");
            }
        }

        class RaisingEncoder extends JsonSerializer {
            override encodeFunctionReturnValue(): string {
                throw new Error("the answer could not be written");
            }
        }

        const module = XPLPC.shared().module;
        const data = '{"f":"platform.serializer.raises","p":[]}';
        const error = vi.spyOn(console, "error").mockImplementation(() => {});

        MappingList.shared().add(
            "platform.serializer.raises",
            new MappingItem(() => Promise.resolve("served")),
        );

        try {
            const decoded = await recordAnswers(() => {
                new Layer(module, new Config(new RaisingDecoder())).invoke(
                    4n,
                    data,
                );
            });

            expect(decoded).toEqual([[4n, ""]]);

            const encoded = await recordAnswers(async () => {
                new Layer(module, new Config(new RaisingEncoder())).invoke(
                    5n,
                    data,
                );

                await new Promise((resolve) => setTimeout(resolve, 0));
            });

            expect(encoded).toEqual([[5n, ""]]);
        } finally {
            error.mockRestore();
        }
    });

    test("answers the caller when the native side cannot be reached", async () => {
        // The module raising on a call is answered like every other failing path rather than rejected or thrown at the caller.

        const bridge = XPLPC.shared().module.JavascriptBridge;
        const call = vi.spyOn(bridge, "call").mockImplementation(() => {
            throw new Error("the module could not be reached");
        });
        const error = vi.spyOn(console, "error").mockImplementation(() => {});

        try {
            expect(
                await Client.call<string>(new Request("sample.login")),
            ).toBeUndefined();
            expect(
                Client.callSync<string>(new Request("sample.login")),
            ).toBeUndefined();
            expect(error).toHaveBeenCalledTimes(2);
        } finally {
            call.mockRestore();
            error.mockRestore();
        }
    });

    test("answers the caller when a serializer raises writing the request", async () => {
        // A serializer of the caller's own can raise while it writes the request, and both entry points answer rather than handing the exception over.

        class RaisingRequestEncoder extends JsonSerializer {
            override encodeRequest(): string {
                throw new Error("the request could not be written");
            }
        }

        const shared = XPLPC.shared();
        const config = shared.config;
        const error = vi.spyOn(console, "error").mockImplementation(() => {});

        try {
            shared.config = new Config(new RaisingRequestEncoder());

            expect(
                await Client.call<string>(new Request("sample.login")),
            ).toBeUndefined();

            expect(
                Client.callSync<string>(new Request("sample.login")),
            ).toBeUndefined();

            expect(error).toHaveBeenCalledTimes(2);
            expect(error.mock.calls[0].join(" ")).toContain(
                "Error when encode the request",
            );
            expect(error.mock.calls[1].join(" ")).toContain(
                "Error when encode the request",
            );
        } finally {
            shared.config = config;
            error.mockRestore();
        }
    });

    test("answers the caller when a serializer raises reading the answer", async () => {
        // The answer is decoded inside the native callback, where what the serializer raises would never settle the promise.

        class RaisingAnswerDecoder extends JsonSerializer {
            override decodeFunctionReturnValue<T>(): T | undefined {
                throw new Error("the answer could not be read");
            }
        }

        const shared = XPLPC.shared();
        const config = shared.config;
        const error = vi.spyOn(console, "error").mockImplementation(() => {});

        MappingList.shared().add(
            "platform.answer.raises",
            new MappingItem(() => Promise.resolve("served")),
        );

        try {
            shared.config = new Config(new RaisingAnswerDecoder());

            expect(
                await Client.call<string>(
                    new Request("platform.answer.raises"),
                ),
            ).toBeUndefined();

            expect(
                Client.callSync<string>(
                    new Request("sample.echo", new Param("value", "x")),
                ),
            ).toBeUndefined();

            expect(error).toHaveBeenCalledTimes(2);
        } finally {
            shared.config = config;
            error.mockRestore();
        }
    });

    test("reports a request it cannot read once", async () => {
        // One event is one line, so the decoder says what failed and the layer answers without repeating it.

        const error = vi.spyOn(console, "error").mockImplementation(() => {});

        try {
            for (const data of ["not-a-json", "null", '{"p":[]}']) {
                error.mockClear();

                const answers = await recordAnswers(() => {
                    hostLayer().invoke(3n, data);
                });

                expect(answers).toEqual([[3n, ""]]);
                expect(error).toHaveBeenCalledTimes(1);
            }
        } finally {
            error.mockRestore();
        }
    });
});
