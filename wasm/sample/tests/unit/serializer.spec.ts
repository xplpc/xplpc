import { describe, expect, it, vi } from "vitest";

import { JsonSerializer } from "@/xplpc/serializer/json-serializer";
import { DataView } from "@/xplpc/type/data-view";
import { Param } from "@/xplpc/message/param";

describe("Serializer", () => {
    const serializer = new JsonSerializer();

    function encodedValue(param: Param): unknown {
        const data = serializer.encodeRequest("sample.wire", param);
        return JSON.parse(data).p[0].v;
    }

    it("sends a data view as pointer and size", () => {
        // The reference format is what the c++ core writes, and every bridge has to agree with it.

        const value = encodedValue(
            new Param("dataView", new DataView(128, 4)),
        ) as Record<string, number>;

        expect(value.ptr).toBe(128);
        expect(value.size).toBe(4);
    });

    it("answers empty for a value it cannot represent", () => {
        // This bridge carries no character and no date type, so a value it cannot represent is reported instead of written wrong.

        const serializer = new JsonSerializer();

        expect(serializer.encodeRequest("f", new Param("big", BigInt(1)))).toBe(
            "",
        );
    });

    it("keeps integers exact up to the safe range", () => {
        expect(encodedValue(new Param("int", Number.MAX_SAFE_INTEGER))).toBe(
            Number.MAX_SAFE_INTEGER,
        );
    });

    it("shows that the plain parser rounds an integer above the safe range", () => {
        // A javascript number is a double, so the plain parser rounds and hands back a different value.

        expect(JSON.parse("9007199254740993")).toBe(9007199254740992);
    });

    it("refuses a decoded integer it cannot represent exactly", () => {
        expect(
            serializer.decodeFunctionReturnValue('{"r":9007199254740993}'),
        ).toBeUndefined();
    });

    it("accepts a decoded integer inside the safe range", () => {
        expect(
            serializer.decodeFunctionReturnValue('{"r":9007199254740991}'),
        ).toBe(9007199254740991);
    });

    it("accepts a decoded floating point value of any magnitude", () => {
        expect(serializer.decodeFunctionReturnValue('{"r":1e300}')).toBe(1e300);
    });
    it("keeps a parameter that has no value", () => {
        const data = serializer.encodeRequest(
            "sample.wire",
            new Param("value", null),
        );
        const param = JSON.parse(data).p[0];

        expect(param.n).toBe("value");
        expect(param.v).toBeNull();
    });
    it("carries no insignificant whitespace", () => {
        // The reference format is what the c++ core writes, and a serializer that spaces its output carries different bytes for the same value.

        const data = serializer.encodeRequest(
            "sample.wire",
            new Param("a", 1),
            new Param("b", "x"),
        );

        expect(data).toBe(
            '{"f":"sample.wire","p":[{"n":"a","v":1},{"n":"b","v":"x"}]}',
        );
    });
    it("carries an empty array for a request with no parameters", () => {
        const data = serializer.encodeRequest("sample.ping");

        expect(JSON.parse(data).p).toEqual([]);
    });
    it("answers empty when the data cannot be read", () => {
        // Every bridge answers the empty value for its type when the data cannot be read.

        expect(serializer.decodeRequest("not-a-json")).toBeUndefined();
        expect(serializer.decodeRequest("{}")?.functionName).toBe("");
        expect(
            serializer.decodeFunctionReturnValue<string>("not-a-json"),
        ).toBeUndefined();
    });

    it("does not report an empty response as a failure", () => {
        // The empty string is what every failing path answers with, so handing it to the parser would report a failure that did not happen.

        const reported = vi
            .spyOn(console, "error")
            .mockImplementation(() => {});

        try {
            expect(
                serializer.decodeFunctionReturnValue<string>(""),
            ).toBeUndefined();
            expect(reported).not.toHaveBeenCalled();

            expect(
                serializer.decodeFunctionReturnValue<string>("not-a-json"),
            ).toBeUndefined();
            expect(reported).toHaveBeenCalled();
        } finally {
            reported.mockRestore();
        }
    });
    it("writes the empty value for a number that is not finite", () => {
        // The wire has no token for infinity, so a value that cannot travel reaches the caller as the empty one.

        expect(serializer.encodeFunctionReturnValue(Infinity)).toBe(
            '{"r":null}',
        );
        expect(serializer.encodeFunctionReturnValue(NaN)).toBe('{"r":null}');
        expect(serializer.encodeFunctionReturnValue(2.5)).toBe('{"r":2.5}');
    });
});
