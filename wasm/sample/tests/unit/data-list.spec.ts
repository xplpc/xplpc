import { describe, expect, it, afterEach } from "vitest";

import { Config } from "@/xplpc/core/config";
import { XPLPC } from "@/xplpc/core/xplpc";
import { MappingList } from "@/xplpc/data/mapping-list";
import { MappingItem } from "@/xplpc/map/mapping-item";
import { Message } from "@/xplpc/message/message";
import { JsonSerializer } from "@/xplpc/serializer/json-serializer";

import Module from "@xplpc/build/wasm/wasm32/bin/xplpc";

describe("Data Lists", () => {
    beforeAll(async () => {
        XPLPC.shared().initialize(
            await Module(),
            new Config(new JsonSerializer()),
        );
    });

    afterEach(() => {
        MappingList.shared().clear();
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

    it("restores the previous mapping when the router refuses the name", () => {
        const first = new MappingItem(() => Promise.resolve("first"));

        MappingList.shared().add("sample.temporary", first);

        const declare = vi
            .spyOn(XPLPC.shared().module.JavascriptBridge, "declare")
            .mockImplementationOnce(() => {
                throw new Error("the router refused the name");
            });

        try {
            expect(() =>
                MappingList.shared().add(
                    "sample.temporary",
                    new MappingItem(() => Promise.resolve("second")),
                ),
            ).toThrow();
        } finally {
            declare.mockRestore();
        }

        expect(MappingList.shared().find("sample.temporary")).toBe(first);
    });

    it("forgets a new mapping when the router refuses the name", () => {
        const declare = vi
            .spyOn(XPLPC.shared().module.JavascriptBridge, "declare")
            .mockImplementationOnce(() => {
                throw new Error("the router refused the name");
            });

        try {
            expect(() =>
                MappingList.shared().add(
                    "sample.refused",
                    new MappingItem(() => Promise.resolve("refused")),
                ),
            ).toThrow();
        } finally {
            declare.mockRestore();
        }

        expect(MappingList.shared().has("sample.refused")).toBe(false);
    });

    it("reads a message value as the requested type", () => {
        const message = new Message();
        message.set("number", 10);

        expect(message.get<number>("number")).toBe(10);
        expect(message.get<number>("missing")).toBeUndefined();
    });
});
