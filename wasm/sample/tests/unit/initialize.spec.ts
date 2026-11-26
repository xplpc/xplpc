import { Client } from "@/xplpc/client/client";
import { Config } from "@/xplpc/core/config";
import { XPLPC } from "@/xplpc/core/xplpc";
import { MappingList } from "@/xplpc/data/mapping-list";
import { MappingItem } from "@/xplpc/map/mapping-item";
import { Param } from "@/xplpc/message/param";
import { Request } from "@/xplpc/message/request";
import { HostLayer, WasmModule } from "@/xplpc/module/xplpc-module";
import { JsonSerializer } from "@/xplpc/serializer/json-serializer";
import { DataView } from "@/xplpc/type/data-view";

import Module from "@xplpc/build/wasm/wasm32/bin/xplpc";

describe("Initialize", () => {
    // This file loads a module of its own, so the library starts here not initialized and the page is not attached to its router.
    let module: WasmModule;
    const config = new Config(new JsonSerializer());

    beforeAll(async () => {
        module = await Module();
    });

    afterEach(() => {
        expect(module.JavascriptBridge.pendingCount()).toBe(0);
    });

    test("declaring a mapping before initialize raises an error naming the library", () => {
        expect(() =>
            MappingList.shared().add(
                "platform.echo",
                new MappingItem(() => Promise.resolve("echo")),
            ),
        ).toThrow(/XPLPC/);

        expect(MappingList.shared().has("platform.echo")).toBe(false);
    });

    test("a data view used before initialize names nothing and says why", () => {
        // A view is created and read through the module, which the library only holds once it is up, so both are refused the way a call is.

        const error = vi.spyOn(console, "error").mockImplementation(() => {});

        try {
            const created = DataView.createFromArrayBuffer(new ArrayBuffer(4));
            const named = new DataView((1n << 32n) | 1n);

            expect(created.handle).toBe(0n);
            expect(
                DataView.withUint8Array(named, (bytes) => bytes.length),
            ).toBe(0);
            expect(error).toHaveBeenCalledTimes(2);
            expect(error).toHaveBeenCalledWith(
                "[XPLPC]",
                expect.stringContaining("not initialized"),
            );
        } finally {
            error.mockRestore();
        }
    });

    test("a call sync before initialize answers empty and says why", () => {
        // A request is written with the config, which the library only holds once it is up, so reading an answer is refused the way a call is.

        const error = vi.spyOn(console, "error").mockImplementation(() => {});

        try {
            expect(
                Client.callSync<string>(new Request("sample.login")),
            ).toBeUndefined();
            expect(Client.callSync('{"f":"sample.login","p":[]}')).toBe("");
            expect(error).toHaveBeenCalledTimes(2);
            expect(error).toHaveBeenCalledWith(
                "[XPLPC]",
                expect.stringContaining("XPLPC was not initialized"),
            );
        } finally {
            error.mockRestore();
        }
    });

    test("an attach that raises leaves the library down and retryable", () => {
        const attach = vi
            .spyOn(module.JavascriptBridge, "attach")
            .mockImplementationOnce(() => {
                throw new Error("the layer could not be attached");
            });

        try {
            expect(() => XPLPC.shared().initialize(module, config)).toThrow();
        } finally {
            attach.mockRestore();
        }

        expect(XPLPC.shared().initialized).toBe(false);
    });

    test("a retry after a failure brings the library up once", async () => {
        XPLPC.shared().initialize(module, config);
        XPLPC.shared().initialize(module, config);

        expect(XPLPC.shared().initialized).toBe(true);

        MappingList.shared().add(
            "platform.echo",
            new MappingItem(() => Promise.resolve("echo")),
        );

        expect(await Client.call<string>(new Request("platform.echo"))).toBe(
            "echo",
        );
        expect(
            await Client.call<string>(
                new Request(
                    "sample.login",
                    new Param("username", "paulo"),
                    new Param("password", "123456"),
                    new Param("remember", true),
                ),
            ),
        ).toBe("LOGGED-WITH-REMEMBER");
    });

    test("attaching a second time keeps the layer the page attached first", async () => {
        // A second attach that replaced the first would route every name declared from now on to the new layer instead.

        const invoke = vi.fn();
        const second: HostLayer = { invoke };

        module.JavascriptBridge.attach(second);

        let answers = 0;

        MappingList.shared().add(
            "platform.once",
            new MappingItem(() => {
                answers += 1;
                return Promise.resolve("once");
            }),
        );

        expect(await Client.call<string>(new Request("platform.once"))).toBe(
            "once",
        );
        expect(answers).toBe(1);
        expect(invoke).not.toHaveBeenCalled();
    });
});
