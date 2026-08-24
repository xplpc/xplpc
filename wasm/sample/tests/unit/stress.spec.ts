import { describe, expect, it, afterEach } from "vitest";

import { CallbackList } from "@/xplpc/data/callback-list";
import { MappingList } from "@/xplpc/data/mapping-list";
import { UniqueID } from "@/xplpc/util/unique-id";

const iterationCount = 2000;

describe("Stress", () => {
    afterEach(() => {
        CallbackList.shared().clear();
        MappingList.shared().clear();
    });

    it("never leaves an entry behind", () => {
        let executed = 0;

        for (let i = 0; i < iterationCount; i++) {
            const key = `stress-${i}`;

            CallbackList.shared().add(key, () => {
                executed += 1;
            });

            CallbackList.shared().execute(key, "data");
        }

        expect(executed).toBe(iterationCount);
        expect(CallbackList.shared().count()).toBe(0);
    });

    it("answers unknown keys without growing", () => {
        for (let i = 0; i < iterationCount; i++) {
            CallbackList.shared().execute(`missing-${i}`, "");
        }

        expect(CallbackList.shared().count()).toBe(0);
    });

    it("resolves each key with its own value when answers interleave", () => {
        // Interleaving means a callback registered later can be answered first.

        const received: Record<string, string> = {};
        const keys: string[] = [];

        for (let i = 0; i < 500; i++) {
            const key = UniqueID.generate();
            keys.push(key);

            CallbackList.shared().add(key, (response: string) => {
                received[key] = response;
            });
        }

        for (const key of [...keys].reverse()) {
            CallbackList.shared().execute(key, `value-${key}`);
        }

        for (const key of keys) {
            expect(received[key]).toBe(`value-${key}`);
        }

        expect(CallbackList.shared().count()).toBe(0);
    });

    it("keeps generating distinct keys under load", () => {
        const keys = new Set(
            Array.from({ length: iterationCount }, () => UniqueID.generate()),
        );

        expect(keys.size).toBe(iterationCount);
    });
});
