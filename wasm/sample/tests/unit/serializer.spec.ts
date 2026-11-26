import { describe, expect, it, vi } from "vitest";

import { JsonSerializer } from "@/xplpc/serializer/json-serializer";
import { DataView } from "@/xplpc/type/data-view";
import { Param } from "@/xplpc/message/param";

describe("Serializer", () => {
    const serializer = new JsonSerializer();

    it("reports a refused parameter as every bridge reports it", () => {
        const error = vi.spyOn(console, "error").mockImplementation(() => {});

        try {
            const cases: [string, string][] = [
                [
                    '{"f":"auth","p":[{"n":"token","v":1},{"n":"token","v":2}]}',
                    'Parameter "token" of function "auth" appears more than once',
                ],
                [
                    '{"f":"auth","p":["token"]}',
                    'Function "auth" carries a parameter whose name is not text',
                ],
            ];

            for (const [data, failure] of cases) {
                error.mockClear();

                expect(serializer.decodeRequest(data)).toBeUndefined();
                expect(error).toHaveBeenCalledTimes(1);
                expect(error).toHaveBeenCalledWith(
                    "[XPLPC]",
                    expect.stringContaining(failure),
                );
            }
        } finally {
            error.mockRestore();
        }
    });

    it("refuses a value the wire has no shape for rather than writing it wrong", () => {
        // A date would go out as text and a map, a set or binary data as an object of their own shape, which no other bridge reads as what was sent.

        const error = vi.spyOn(console, "error").mockImplementation(() => {});

        try {
            const unwritable = [
                new Date(0),
                new Map([["a", 1]]),
                new Set([1]),
                new Uint8Array([1, 2]),
                new ArrayBuffer(2),
            ];

            for (const value of unwritable) {
                expect(
                    serializer.encodeRequest(
                        "sample.wire",
                        new Param("v", value),
                    ),
                ).toBe("");
                expect(
                    serializer.encodeFunctionReturnValue({ nested: [value] }),
                ).toBe("");
            }
        } finally {
            error.mockRestore();
        }
    });

    it("writes a parameter without a value and an answer of nothing as null", () => {
        expect(
            serializer.encodeRequest("sample.wire", new Param("nothing")),
        ).toBe('{"f":"sample.wire","p":[{"n":"nothing","v":null}]}');
        expect(serializer.encodeFunctionReturnValue(undefined)).toBe(
            '{"r":null}',
        );
    });

    it("sends a data view as its buffer handle and nothing else", () => {
        // The reference format is what the c++ core writes, and every bridge has to agree with it.

        const handle = (5n << 32n) | 1n;
        const data = serializer.encodeRequest(
            "sample.wire",
            new Param("dataView", new DataView(handle)),
        );

        expect(data).toBe(
            '{"f":"sample.wire","p":[{"n":"dataView","v":{"buffer":21474836481}}]}',
        );
    });

    it("answers empty for a handle past what a javascript number holds", () => {
        // Writing that handle as a number would round it into one naming another buffer, so the request is refused instead.

        const error = vi.spyOn(console, "error").mockImplementation(() => {});

        try {
            expect(
                serializer.encodeRequest(
                    "sample.wire",
                    new Param(
                        "dataView",
                        new DataView(BigInt(Number.MAX_SAFE_INTEGER) + 2n),
                    ),
                ),
            ).toBe("");
            expect(error).toHaveBeenCalledTimes(1);
        } finally {
            error.mockRestore();
        }
    });

    it("answers empty for a value it cannot represent", () => {
        // This bridge carries no character and no date type, so a value it cannot represent is reported instead of written wrong.

        const serializer = new JsonSerializer();

        expect(serializer.encodeRequest("f", new Param("big", BigInt(1)))).toBe(
            "",
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

    it("answers a document that carries no value without reporting it", () => {
        // Only a document that cannot be read is a failure, so a root that is not an object is the empty value rather than a parse error.

        const reported = vi
            .spyOn(console, "error")
            .mockImplementation(() => {});

        try {
            for (const data of ["null", "{}", "[1]", "5", '"r"']) {
                expect(
                    serializer.decodeFunctionReturnValue(data),
                ).toBeUndefined();
            }

            expect(reported).not.toHaveBeenCalled();
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

    describe("on a runtime that does not hand the reviver the source text", () => {
        // Older engines call the reviver with two arguments, so the guard sees only the number it has already been given.

        let parse: { mockRestore(): void };

        beforeEach(() => {
            const original = JSON.parse;

            parse = vi
                .spyOn(JSON, "parse")
                .mockImplementation(
                    (
                        text: string,
                        reviver?: (
                            this: unknown,
                            key: string,
                            value: unknown,
                        ) => unknown,
                    ) =>
                        original(
                            text,
                            reviver
                                ? function (
                                      this: unknown,
                                      key: string,
                                      value: unknown,
                                  ) {
                                      return reviver.call(this, key, value);
                                  }
                                : undefined,
                        ),
                );
        });

        afterEach(() => {
            parse.mockRestore();
        });

        it("refuses an integer past the safe range rather than rounding it", () => {
            const error = vi
                .spyOn(console, "error")
                .mockImplementation(() => {});

            try {
                expect(
                    serializer.decodeFunctionReturnValue(
                        '{"r":9007199254740993}',
                    ),
                ).toBeUndefined();
                expect(
                    serializer.decodeFunctionReturnValue(
                        '{"r":-9007199254740993}',
                    ),
                ).toBeUndefined();
                expect(
                    serializer.decodeFunctionReturnValue(
                        '{"r":{"a":[1,9007199254740993]}}',
                    ),
                ).toBeUndefined();
                expect(
                    serializer.decodeRequest(
                        '{"f":"x","p":[{"n":"a","v":9007199254740993}]}',
                    ),
                ).toBeUndefined();
            } finally {
                error.mockRestore();
            }
        });

        it("accepts an integer inside the safe range and a fraction", () => {
            expect(
                serializer.decodeFunctionReturnValue('{"r":9007199254740991}'),
            ).toBe(9007199254740991);
            expect(
                serializer.decodeFunctionReturnValue('{"r":-9007199254740991}'),
            ).toBe(-9007199254740991);
            expect(serializer.decodeFunctionReturnValue('{"r":2.5}')).toBe(2.5);
        });

        it("refuses a large whole valued fraction too, since nothing tells it from an integer", () => {
            const error = vi
                .spyOn(console, "error")
                .mockImplementation(() => {});

            try {
                expect(
                    serializer.decodeFunctionReturnValue('{"r":1e300}'),
                ).toBeUndefined();
            } finally {
                error.mockRestore();
            }
        });
    });

    it("refuses a request that names a parameter twice", () => {
        // A name read twice means the first value to one bridge and the last to another, so the only answer every bridge can agree on is refusing it.

        const error = vi.spyOn(console, "error").mockImplementation(() => {});

        try {
            expect(
                serializer.decodeRequest(
                    '{"f":"auth","p":[{"n":"token","v":"allow"},{"n":"token","v":"deny"}]}',
                ),
            ).toBeUndefined();
            expect(error).toHaveBeenCalledTimes(1);
            expect(String(error.mock.calls[0][1])).toContain('"auth"');
            expect(String(error.mock.calls[0][1])).not.toContain("allow");
        } finally {
            error.mockRestore();
        }
    });
});
