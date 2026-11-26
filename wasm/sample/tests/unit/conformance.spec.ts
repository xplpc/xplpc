import { vi } from "vitest";

import { Config } from "@/xplpc/core/config";
import { XPLPC } from "@/xplpc/core/xplpc";
import { Param } from "@/xplpc/message/param";
import { Request } from "@/xplpc/message/request";
import { JsonSerializer } from "@/xplpc/serializer/json-serializer";
import { DataView } from "@/xplpc/type/data-view";

import Module from "@xplpc/build/wasm/wasm32/bin/xplpc";

interface WireCase {
    function: string;
    params: { name: string; type: string; value: unknown }[];
    bytes: string;
}

interface AcceptCase {
    document: string;
    function: string;
    params: Record<string, unknown>;
}

interface RefuseCase {
    document: string;
}

// Every bridge runs the same cases, which live at the root of the repository and are listed when the suite is collected, so a case added there needs no code here.
const files = import.meta.glob<string>("@xplpc/conformance/*/*.json", {
    eager: true,
    query: "?raw",
    import: "default",
});

function casesOf<T>(group: string): [string, T][] {
    const cases: [string, T][] = [];

    for (const [path, text] of Object.entries(files)) {
        const match = /([^/]+)\/([^/]+)\.json$/.exec(path);

        if (match?.[1] !== group) {
            continue;
        }

        const testCase: T = JSON.parse(text);
        cases.push([match[2], testCase]);
    }

    return cases.sort(([a], [b]) => a.localeCompare(b));
}

// A javascript number holds an integer exactly only up to two to the fifty three, and the page carries no character or date type, so a caller sends the code point or the milliseconds as a number.
const notSent: Record<string, string> = {
    int64: "a javascript number cannot hold the full 64 bit range",
    uint64: "a javascript number cannot hold the full 64 bit range",
    char: "the page carries no character type",
    date: "the page carries no date type",
};

function isSent(testCase: WireCase): boolean {
    return testCase.params.every((param) => !(param.type in notSent));
}

function valueOf(param: WireCase["params"][number]): unknown {
    if (param.type === "dataview" && typeof param.value === "number") {
        return new DataView(BigInt(param.value));
    }

    if (param.type === "list<dataview>" && Array.isArray(param.value)) {
        return param.value.map((handle: unknown) =>
            typeof handle === "number" ? new DataView(BigInt(handle)) : handle,
        );
    }

    return param.value;
}

describe("Conformance", () => {
    beforeAll(async () => {
        XPLPC.shared().initialize(
            await Module(),
            new Config(new JsonSerializer()),
        );
    });

    describe("wire", () => {
        const wire = casesOf<WireCase>("wire");

        test.each(wire.filter(([, testCase]) => isSent(testCase)))(
            "%s",
            (_, testCase) => {
                const params = testCase.params.map(
                    (param) => new Param(param.name, valueOf(param)),
                );
                const request = new Request(testCase.function, ...params);

                expect(request.data()).toBe(testCase.bytes);
            },
        );

        test.skip.each(wire.filter(([, testCase]) => !isSent(testCase)))(
            "%s",
            () => {},
        );
    });

    describe("accept", () => {
        test.each(casesOf<AcceptCase>("accept"))("%s", (_, testCase) => {
            const decoded = new JsonSerializer().decodeRequest(
                testCase.document,
            );

            expect(decoded).toBeDefined();
            expect(decoded?.functionName).toBe(testCase.function);

            // A parameter that arrived without a value reads as undefined, which is how this bridge says there is none.
            for (const [name, value] of Object.entries(testCase.params)) {
                expect(decoded?.message.get(name) ?? null).toEqual(value);
            }
        });
    });

    describe("refuse", () => {
        test.each(casesOf<RefuseCase>("refuse"))("%s", (_, testCase) => {
            const error = vi
                .spyOn(console, "error")
                .mockImplementation(() => {});

            try {
                const decoded = new JsonSerializer().decodeRequest(
                    testCase.document,
                );

                expect(decoded).toBeUndefined();
                expect(error).toHaveBeenCalledTimes(1);
            } finally {
                error.mockRestore();
            }
        });
    });
});
