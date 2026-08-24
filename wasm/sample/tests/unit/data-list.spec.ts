import { describe, expect, it, afterEach } from "vitest";

import { CallbackList } from "@/xplpc/data/callback-list";
import { MappingList } from "@/xplpc/data/mapping-list";
import { MappingItem } from "@/xplpc/map/mapping-item";
import { Message } from "@/xplpc/message/message";
import { DataView } from "@/xplpc/type/data-view";
import { UniqueID } from "@/xplpc/util/unique-id";

describe("Data Lists", () => {
    afterEach(() => {
        CallbackList.shared().clear();
        MappingList.shared().clear();
    });

    it("registers a callback synchronously", () => {
        CallbackList.shared().add("sync-key", () => {});

        expect(CallbackList.shared().count()).toBe(1);
    });

    it("executes a callback only once", () => {
        let calls = 0;

        CallbackList.shared().add("once-key", () => {
            calls += 1;
        });

        CallbackList.shared().execute("once-key", "");
        CallbackList.shared().execute("once-key", "");

        expect(calls).toBe(1);
    });

    it("answers an unknown key without touching the list", () => {
        CallbackList.shared().execute("unknown-key", "");

        expect(CallbackList.shared().count()).toBe(0);
    });

    it("drops a pending callback on remove", () => {
        let calls = 0;

        CallbackList.shared().add("removed-key", () => {
            calls += 1;
        });

        CallbackList.shared().remove("removed-key");
        CallbackList.shared().execute("removed-key", "");

        expect(calls).toBe(0);
        expect(CallbackList.shared().count()).toBe(0);
    });

    it("resolves the callback registered before an immediate answer", () => {
        let received = "";

        // The wasm module answers inside the same tick, so an asynchronous registration would miss it.
        const key = UniqueID.generate();

        CallbackList.shared().add(key, (response: string) => {
            received = response;
        });

        CallbackList.shared().execute(key, "immediate");

        expect(received).toBe("immediate");
    });

    it("generates distinct keys", () => {
        const keys = new Set(
            Array.from({ length: 1000 }, () => UniqueID.generate()),
        );

        expect(keys.size).toBe(1000);
    });

    it("keeps the mapping list consistent", () => {
        MappingList.shared().add(
            "sample.temporary",
            new MappingItem(() => Promise.resolve("")),
        );

        expect(MappingList.shared().has("sample.temporary")).toBe(true);
        expect(MappingList.shared().count()).toBe(1);

        MappingList.shared().clear();

        expect(MappingList.shared().has("sample.temporary")).toBe(false);
        expect(MappingList.shared().count()).toBe(0);
    });

    it("reads a message value as the requested type", () => {
        const message = new Message();
        message.set("number", 10);

        expect(message.get<number>("number")).toBe(10);
        expect(message.get<number>("missing")).toBeUndefined();
    });
    it("reads nothing from a view without an address", () => {
        // A view that carries no address describes nothing to read, whatever length it claims.

        DataView.withUint8Array(new DataView(0, 16), (bytes) => {
            expect(bytes.length).toBe(0);
        });

        DataView.withUint8Array(new DataView(128, 0), (bytes) => {
            expect(bytes.length).toBe(0);
        });
    });
});
