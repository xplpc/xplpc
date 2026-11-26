import { Client } from "@/xplpc/client/client";
import { Config } from "@/xplpc/core/config";
import { XPLPC } from "@/xplpc/core/xplpc";
import { MappingList } from "@/xplpc/data/mapping-list";
import { MappingItem } from "@/xplpc/map/mapping-item";
import { Param } from "@/xplpc/message/param";
import { Request } from "@/xplpc/message/request";
import { JsonSerializer } from "@/xplpc/serializer/json-serializer";

import Module from "@xplpc/build/wasm/wasm32/bin/xplpc";

function loginRequest(): Request {
    return new Request(
        "sample.login",
        new Param("username", "paulo"),
        new Param("password", "123456"),
        new Param("remember", true),
    );
}

function neverAnswering(): Promise<string> {
    return new Promise<string>(() => {});
}

describe("Router", () => {
    beforeAll(async () => {
        XPLPC.shared().initialize(
            await Module(),
            new Config(new JsonSerializer()),
        );
    });

    afterEach(() => {
        MappingList.shared().clear();

        expect(XPLPC.shared().module.JavascriptBridge.pendingCount()).toBe(0);
    });

    test("a page mapping declared under a c++ name receives the call and clearing gives it back", async () => {
        // The last layer to declare a name owns it, so the page takes the name over from c++ and retracting it hands it back.

        MappingList.shared().add(
            "sample.login",
            new MappingItem(() => Promise.resolve("FROM-PAGE")),
        );

        expect(await Client.call<string>(loginRequest())).toBe("FROM-PAGE");

        MappingList.shared().clear();

        expect(await Client.call<string>(loginRequest())).toBe(
            "LOGGED-WITH-REMEMBER",
        );
        expect(Client.callSync<string>(loginRequest())).toBe(
            "LOGGED-WITH-REMEMBER",
        );
    });

    test("a call sync to a name the page took over answers empty", () => {
        // Once the page owns the name the answer is a promise, so it cannot be there by the time the call returns.

        const error = vi.spyOn(console, "error").mockImplementation(() => {});

        MappingList.shared().add(
            "sample.login",
            new MappingItem(() => Promise.resolve("FROM-PAGE")),
        );

        try {
            expect(Client.callSync<string>(loginRequest())).toBeUndefined();
        } finally {
            error.mockRestore();
        }
    });

    test("an inline answer runs the callback before the call returns and carries no handle", () => {
        const bridge = XPLPC.shared().module.JavascriptBridge;
        const responses: string[] = [];

        const call = bridge.call(loginRequest().data(), (response) => {
            responses.push(response);
        });

        expect(call).toBe(0n);
        expect(responses).toEqual(['{"r":"LOGGED-WITH-REMEMBER"}']);
        expect(bridge.cancel(call)).toBe(false);
    });

    test("a pending call is a bigint handle that cancel takes back", () => {
        MappingList.shared().add(
            "platform.never",
            new MappingItem(neverAnswering),
        );

        const bridge = XPLPC.shared().module.JavascriptBridge;
        const callback = vi.fn();
        const call = bridge.call('{"f":"platform.never","p":[]}', callback);

        expect(typeof call).toBe("bigint");
        expect(call).not.toBe(0n);
        expect(bridge.pendingCount()).toBe(1);

        expect(bridge.cancel(call)).toBe(true);
        expect(bridge.cancel(call)).toBe(false);
        expect(bridge.pendingCount()).toBe(0);

        // A cancelled call is no longer in the router, so answering it is a no-op that never reaches the callback.
        bridge.answer(call, '{"r":"late"}');

        expect(callback).not.toHaveBeenCalled();
    });

    test("a call answered twice resolves once", () => {
        MappingList.shared().add(
            "platform.never",
            new MappingItem(neverAnswering),
        );

        const bridge = XPLPC.shared().module.JavascriptBridge;
        const callback = vi.fn();
        const call = bridge.call('{"f":"platform.never","p":[]}', callback);

        bridge.answer(call, '{"r":"first"}');
        bridge.answer(call, '{"r":"second"}');

        expect(callback).toHaveBeenCalledTimes(1);
        expect(callback).toHaveBeenCalledWith('{"r":"first"}');
    });

    test("a handle whose slot was reused is refused", () => {
        // The handle carries the slot and its generation, so an old handle cannot resolve the newer call that took its slot.

        MappingList.shared().add(
            "platform.never",
            new MappingItem(neverAnswering),
        );

        const bridge = XPLPC.shared().module.JavascriptBridge;
        const older = bridge.call('{"f":"platform.never","p":[]}', () => {});

        bridge.cancel(older);

        const newer = vi.fn();
        const call = bridge.call('{"f":"platform.never","p":[]}', newer);

        expect(call >> 32n).toBe(older >> 32n);
        expect(call).not.toBe(older);

        bridge.answer(older, '{"r":"stale"}');

        expect(newer).not.toHaveBeenCalled();
        expect(bridge.pendingCount()).toBe(1);

        bridge.cancel(call);
    });

    test("a call routed while the name is being declared already finds the mapping", async () => {
        // Nothing in this module can route a call during a declare, so a declare that routes one stands in for the threaded bridges where the router can.

        const bridge = XPLPC.shared().module.JavascriptBridge;
        const declare = bridge.declare;
        let routed: Promise<string | undefined> | undefined;

        bridge.declare = (name: string) => {
            declare(name);
            routed = Client.call<string>(new Request(name));
        };

        try {
            MappingList.shared().add(
                "platform.declaring",
                new MappingItem(() => Promise.resolve("declared")),
            );
        } finally {
            bridge.declare = declare;
        }

        expect(await routed).toBe("declared");
    });
});
