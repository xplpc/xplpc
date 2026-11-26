import { describe, expect, it, afterEach } from "vitest";

import { Config } from "@/xplpc/core/config";
import { XPLPC } from "@/xplpc/core/xplpc";
import { MappingList } from "@/xplpc/data/mapping-list";
import { MappingItem } from "@/xplpc/map/mapping-item";
import { Param } from "@/xplpc/message/param";
import { Request } from "@/xplpc/message/request";
import { JsonSerializer } from "@/xplpc/serializer/json-serializer";

import Module from "@xplpc/build/wasm/wasm32/bin/xplpc";

const iterationCount = 2000;

// Every unknown name writes an error line, so this loop is kept short enough for the log to stay readable.
const unknownCount = 200;

// The module built under the address sanitizer takes about twelve milliseconds per call, which is far past the default timeout.
const sanitizedTimeout = 120000;

const neverRequest = '{"f":"platform.never","p":[]}';

describe("Stress", { timeout: sanitizedTimeout }, () => {
    beforeAll(async () => {
        XPLPC.shared().initialize(
            await Module(),
            new Config(new JsonSerializer()),
        );
    });

    beforeEach(() => {
        MappingList.shared().add(
            "platform.never",
            new MappingItem(() => new Promise<string>(() => {})),
        );
    });

    afterEach(() => {
        MappingList.shared().clear();

        expect(XPLPC.shared().module.JavascriptBridge.pendingCount()).toBe(0);
    });

    it("never leaves a call behind when every answer is inline", () => {
        const bridge = XPLPC.shared().module.JavascriptBridge;
        const data = new Request(
            "sample.login",
            new Param("username", "paulo"),
            new Param("password", "123456"),
            new Param("remember", true),
        ).data();

        let executed = 0;

        for (let i = 0; i < iterationCount; i++) {
            const call = bridge.call(data, () => {
                executed += 1;
            });

            expect(call).toBe(0n);
        }

        expect(executed).toBe(iterationCount);
    });

    it("answers unknown names without growing", () => {
        const bridge = XPLPC.shared().module.JavascriptBridge;
        const responses: string[] = [];

        for (let i = 0; i < unknownCount; i++) {
            bridge.call(`{"f":"missing.${i}","p":[]}`, (response) => {
                responses.push(response);
            });
        }

        expect(responses.length).toBe(unknownCount);
        expect(responses.every((response) => response === "")).toBe(true);
    });

    it("resolves each call with its own value when answers interleave", () => {
        // Interleaving means a call made later can be answered first.

        const bridge = XPLPC.shared().module.JavascriptBridge;
        const received = new Map<bigint, string>();
        const calls: bigint[] = [];

        for (let i = 0; i < 500; i++) {
            const call: bigint = bridge.call(neverRequest, (response) => {
                received.set(call, response);
            });

            calls.push(call);
        }

        expect(bridge.pendingCount()).toBe(500);

        for (const call of [...calls].reverse()) {
            bridge.answer(call, `value-${call}`);
        }

        for (const call of calls) {
            expect(received.get(call)).toBe(`value-${call}`);
        }
    });

    it("keeps handing out distinct handles under load", () => {
        const bridge = XPLPC.shared().module.JavascriptBridge;
        const calls = Array.from({ length: iterationCount }, () =>
            bridge.call(neverRequest, () => {}),
        );

        expect(new Set(calls).size).toBe(iterationCount);

        for (const call of calls) {
            expect(bridge.cancel(call)).toBe(true);
        }
    });
});
