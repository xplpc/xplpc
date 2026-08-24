import { Client } from "@/xplpc/client/client";
import { Config } from "@/xplpc/core/config";
import { XPLPC } from "@/xplpc/core/xplpc";
import { CallbackList } from "@/xplpc/data/callback-list";
import { MappingList } from "@/xplpc/data/mapping-list";
import { MappingItem } from "@/xplpc/map/mapping-item";
import { Message } from "@/xplpc/message/message";
import { Param } from "@/xplpc/message/param";
import { Request } from "@/xplpc/message/request";
import { WasmModule } from "@/xplpc/module/xplpc-module";
import platformProxy from "@/xplpc/proxy/platform-proxy";
import { JsonSerializer } from "@/xplpc/serializer/json-serializer";
import { DataView } from "@/xplpc/type/data-view";

import Module from "@xplpc/build/wasm/wasm32/bin/xplpc";

function batteryLevel(message: Message): Promise<string> {
    return new Promise<string>((resolve) => {
        const suffix = message.get("suffix");
        return resolve("100" + suffix);
    });
}

function deferredReverse(): Promise<string> {
    return new Promise<string>((resolve) => {
        setTimeout(() => resolve("ok"), 50);
    });
}

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

function reverseResponse(): Promise<string> {
    return new Promise<string>((resolve) => {
        return resolve("ok");
    });
}

describe("Client", () => {
    afterEach(() => {
        // A leaked callback is the defect this project has found most often, so every test is held to leaving none.
        expect(CallbackList.shared().count()).toBe(0);
    });

    beforeAll(async () => {
        const module: WasmModule = await Module();
        XPLPC.shared().initialize(module, new Config(new JsonSerializer()));

        MappingList.shared().add(
            "platform.battery.level",
            new MappingItem(batteryLevel),
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

    it("check if is initialized", () => {
        expect(XPLPC.shared().initialized).toBeTruthy();
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

    test("battery level from string", () => {
        const request = new Request(
            "platform.battery.level",
            new Param("suffix", "%"),
        );

        return Client.call(request.data()).then((response: string) => {
            expect(response).toBe('{"r":"100%"}');
        });
    });

    test("battery level from string with await", async () => {
        const request = new Request(
            "platform.battery.level",
            new Param("suffix", "%"),
        );
        const response = await Client.call(request.data());
        expect(response).toBe('{"r":"100%"}');
    });

    it("get logged value", () => {
        const request = new Request(
            "sample.login",
            new Param("username", "paulo"),
            new Param("password", "123456"),
            new Param("remember", true),
        );

        return Client.call<string>(request).then(
            (response: string | undefined) => {
                expect(response).toBe("LOGGED-WITH-REMEMBER");
            },
        );
    });

    it("get logged value with await", async () => {
        const request = new Request(
            "sample.login",
            new Param("username", "paulo"),
            new Param("password", "123456"),
            new Param("remember", true),
        );

        const response = await Client.call<string>(request);
        expect(response).toBe("LOGGED-WITH-REMEMBER");
    });

    it("get reverse value", () => {
        const request = new Request("sample.reverse");

        return Client.call<string>(request).then(
            (response: string | undefined) => {
                expect(response).toBe("response-is-ok");
            },
        );
    });

    it("get reverse value with await", async () => {
        const request = new Request("sample.reverse");

        const response = await Client.call<string>(request);
        expect(response).toBe("response-is-ok");
    });

    test("transfer data view", () => {
        const dataView = DataView.createFromArrayBufferView(
            new Uint8Array([
                255,
                0,
                0,
                255, // red pixel
                0,
                255,
                0,
                255, // green pixel
                0,
                0,
                255,
                255, // blue pixel
                0,
                0,
                0,
                0, // transparent pixel
            ]),
        );

        const request = new Request(
            "sample.image.grayscale.dataview",
            new Param("dataView", dataView),
        );

        return Client.call<string>(request).then(
            (response: string | undefined) => {
                expect("OK").toBe(response);

                DataView.withUint8ClampedArray(dataView, (processedData) => {
                    expect(16).toBe(processedData.length);
                    expect(processedData[0]).toBe(85);
                    expect(processedData[4]).toBe(85);
                    expect(processedData[8]).toBe(85);
                    expect(processedData[12]).toBe(0);
                });

                DataView.free(dataView);
            },
        );
    });

    test("transfer data view with await", async () => {
        const dataView = DataView.createFromArrayBufferView(
            new Uint8Array([
                255,
                0,
                0,
                255, // red pixel
                0,
                255,
                0,
                255, // green pixel
                0,
                0,
                255,
                255, // blue pixel
                0,
                0,
                0,
                0, // transparent pixel
            ]),
        );

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

        DataView.free(dataView);
    });

    test("receive data view", () => {
        const request = new Request("sample.dataview");

        return Client.call<DataView>(request).then(
            (response: DataView | undefined) => {
                const dataView = response;

                expect(dataView).toBeDefined();

                if (!dataView) {
                    return;
                }

                DataView.withUint8Array(dataView, (originalData) => {
                    expect(16).toBe(originalData.length);
                    expect(originalData[0]).toBe(255);
                    expect(originalData[5]).toBe(255);
                    expect(originalData[10]).toBe(255);
                    expect(originalData[12]).toBe(0);
                });

                const request = new Request(
                    "sample.image.grayscale.dataview",
                    new Param("dataView", dataView),
                );

                return Client.call<string>(request).then(
                    (response: string | undefined) => {
                        expect("OK").toBe(response);

                        // The mapping wrote into the same block, so reading it again shows the processed bytes.
                        DataView.withUint8Array(dataView, (processedData) => {
                            expect(16).toBe(processedData.length);
                            expect(processedData[0]).toBe(85);
                            expect(processedData[4]).toBe(85);
                            expect(processedData[8]).toBe(85);
                            expect(processedData[12]).toBe(0);
                        });
                    },
                );
            },
        );
    });

    test("receive data view with await", async () => {
        const request = new Request("sample.dataview");
        const response = await Client.call<DataView>(request);

        const dataView = response;

        expect(dataView).toBeDefined();

        if (!dataView) {
            return;
        }

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

        const dataView = await Client.call<DataView>(
            new Request("sample.dataview"),
        );

        expect(dataView).toBeDefined();

        if (!dataView) {
            return;
        }

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
        // A view that was never allocated here is not ours to release.

        const borrowed = await Client.call<DataView>(
            new Request("sample.dataview"),
        );

        expect(borrowed).toBeDefined();

        if (!borrowed) {
            return;
        }

        const ptr = borrowed.ptr;

        DataView.free(borrowed);

        expect(borrowed.ptr).toBe(ptr);
        expect(borrowed.size).toBe(16);
    });

    test("a data view puts only the address and the length on the wire", () => {
        // The wire carries the address and the length, never the bookkeeping the view keeps for itself.

        const dataView = DataView.createFromArrayBuffer(new ArrayBuffer(8));

        expect(JSON.stringify(dataView)).toBe(
            `{"ptr":${dataView.ptr},"size":8}`,
        );

        DataView.free(dataView);
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
        expect(dataView.size).toBe(size);

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
    test("a call for a name the host does not own is answered", async () => {
        // The native side routes by a set the host declared, so a name dropped between the routing and the call still has to be answered rather than left waiting.

        const module = XPLPC.shared().module;
        const original = module.CallbackList.executeFromJavascript;
        const answers: [string, string][] = [];

        module.CallbackList.executeFromJavascript = (
            key: string,
            data: string,
        ) => {
            answers.push([key, data]);
        };

        try {
            platformProxy.callProxy(
                "JS-not-owned",
                '{"f":"host.name.nothing.owns","p":[]}',
            );
        } finally {
            module.CallbackList.executeFromJavascript = original;
        }

        expect(answers).toEqual([["JS-not-owned", ""]]);
    });

    test("a call reaching the host before it is ready is answered", () => {
        // The module holds the proxy from the moment it loads, so a call arriving before the page brought the library up answers rather than hangs.

        const shared = XPLPC.shared();
        const module = shared.module;
        const original = module.CallbackList.executeFromJavascript;
        const answers: [string, string][] = [];

        module.CallbackList.executeFromJavascript = (
            key: string,
            data: string,
        ) => {
            answers.push([key, data]);
        };

        shared.initialized = false;

        try {
            platformProxy.callProxy(
                "JS-not-ready",
                '{"f":"platform.battery.level","p":[]}',
            );
        } finally {
            shared.initialized = true;
            module.CallbackList.executeFromJavascript = original;
        }

        expect(answers).toEqual([["JS-not-ready", ""]]);
    });

    test("a mapping whose promise rejects still answers the caller", async () => {
        MappingList.shared().add(
            "platform.rejecting",
            new MappingItem(rejectingAfterReturning),
        );

        const module = XPLPC.shared().module;
        const original = module.CallbackList.executeFromJavascript;
        const answers: [string, string][] = [];

        module.CallbackList.executeFromJavascript = (
            key: string,
            data: string,
        ) => {
            answers.push([key, data]);
        };

        try {
            platformProxy.callProxy(
                "JS-rejecting",
                '{"f":"platform.rejecting","p":[]}',
            );

            await new Promise((resolve) => setTimeout(resolve, 0));
        } finally {
            module.CallbackList.executeFromJavascript = original;
        }

        expect(answers).toEqual([["JS-rejecting", ""]]);
    });

    test("host mapping that raises before returning still answers the caller", async () => {
        MappingList.shared().add(
            "platform.raising",
            new MappingItem(raisingBeforeReturning),
        );

        const before = CallbackList.shared().count();
        const response = await Client.call<string>(
            new Request("platform.raising"),
        );

        expect(response).toBeUndefined();
        expect(CallbackList.shared().count()).toBe(before);
    });

    test("nested host mapping answering later reaches the outer caller", async () => {
        MappingList.shared().add(
            "platform.reverse.response",
            new MappingItem(deferredReverse),
        );

        const response = await Client.call<string>(
            new Request("sample.reverse"),
        );

        expect(response).toBe("response-is-ok");
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
        expect(await pending).toBe("deferred");
    });

    test("answers empty for a function nothing owns and leaves nothing registered", async () => {
        // Nothing owns this name, so the caller is answered with the empty value and the registration is not left behind.

        const before = CallbackList.shared().count();

        const response = await Client.call<string>(new Request("not.found"));

        expect(response).toBeUndefined();
        expect(CallbackList.shared().count()).toBe(before);
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

    test("call sync answers empty for a host mapping and leaves nothing registered", () => {
        // A mapping this bridge owns answers with a promise, which resolves in a microtask and never inside the call.

        const before = CallbackList.shared().count();
        const error = vi.spyOn(console, "error").mockImplementation(() => {});

        try {
            const request = new Request(
                "platform.battery.level",
                new Param("suffix", "%"),
            );

            expect(Client.callSync<string>(request)).toBeUndefined();
            expect(Client.callSync(request.data())).toBe("");
            expect(CallbackList.shared().count()).toBe(before);
            expect(error).toHaveBeenCalledWith(
                "[XPLPC]",
                expect.stringContaining("did not answer synchronously"),
            );
        } finally {
            error.mockRestore();
        }
    });

    test("call sync answers empty for a function nothing owns and leaves nothing registered", () => {
        const before = CallbackList.shared().count();
        const error = vi.spyOn(console, "error").mockImplementation(() => {});

        try {
            expect(
                Client.callSync<string>(new Request("not.found")),
            ).toBeUndefined();
            expect(Client.callSync('{"f":"not.found","p":[]}')).toBe("");
            expect(CallbackList.shared().count()).toBe(before);
        } finally {
            error.mockRestore();
        }
    });

    test("call sync answers empty when the library was not initialized", () => {
        const shared = XPLPC.shared();
        shared.initialized = false;
        const error = vi.spyOn(console, "error").mockImplementation(() => {});

        try {
            expect(
                Client.callSync<string>(new Request("sample.login")),
            ).toBeUndefined();
        } finally {
            error.mockRestore();
            shared.initialized = true;
        }
    });
    test("reports a request it cannot read once", () => {
        // One event is one line, so the decoder says what failed and the proxy answers without repeating it.

        const error = vi.spyOn(console, "error").mockImplementation(() => {});

        try {
            for (const data of ["not-a-json", "null", '{"p":[]}']) {
                error.mockClear();

                platformProxy.callProxy("k", data);

                expect(error).toHaveBeenCalledTimes(1);
            }
        } finally {
            error.mockRestore();
        }
    });
});
