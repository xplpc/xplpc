import XCTest
import xplpc

final class SerializerTest: XCTestCase {
    private func encodedParams(_ params: [Param]) throws -> [[String: Any]] {
        let data = JsonSerializer().encodeRequest("sample.wire", params)
        let json = try XCTUnwrap(
            JSONSerialization.jsonObject(with: Data(data.utf8)) as? [String: Any]
        )

        return try XCTUnwrap(json["p"] as? [[String: Any]])
    }

    func testCharacterTravelsAsACodePointNumber() throws {
        // The reference format is what the c++ core writes, and every bridge has to agree with it.

        let params = try encodedParams([Param("char", CodePoint("z"))])

        XCTAssertEqual(122, params[0]["v"] as? UInt32)
    }

    func testDateTravelsAsWholeMillisecondsSinceEpoch() throws {
        let date = Date(timeIntervalSince1970: 494_938.8)
        let params = try encodedParams([Param("date", AnyCodable(date))])

        XCTAssertEqual(494_938_800, params[0]["v"] as? Int64)
    }

    func testDataViewTravelsAsPointerAndSize() throws {
        var bytes: [UInt8] = [1, 2, 3, 4]

        try DataView.withByteArray(&bytes) { dataView in
            let params = try encodedParams([Param("dataView", dataView)])
            let value = try XCTUnwrap(params[0]["v"] as? [String: Any])

            XCTAssertNotNil(value["ptr"])
            XCTAssertEqual(4, value["size"] as? UInt)
        }
    }

    func testCodePointRejectsAClusterItCannotRepresent() {
        XCTAssertNil(CodePoint("👨‍👩‍👧‍👦"))
        XCTAssertNotNil(CodePoint("é"))
    }

    func testCodePointRoundTrips() throws {
        let encoded = try JSONEncoder().encode(CodePoint("®"))
        let decoded = try JSONDecoder().decode(CodePoint.self, from: encoded)

        XCTAssertEqual("174", String(data: encoded, encoding: .utf8))
        XCTAssertEqual("®", decoded.character)
    }

    func testIntegerKeepsItsPrecision() throws {
        let params = try encodedParams([Param("int64", AnyCodable(Int64(9_007_199_254_740_993)))])

        XCTAssertEqual(9_007_199_254_740_993, params[0]["v"] as? Int64)
    }

    func testNullValueIsPreserved() throws {
        let params = try encodedParams([Param("value", nil)])

        XCTAssertEqual("value", params[0]["n"] as? String)
        XCTAssertTrue(params[0]["v"] is NSNull)
    }

    func testTheWireCarriesNoInsignificantWhitespace() {
        // The reference format is what the c++ core writes, and a serializer that spaces its output carries different bytes for the same value.
        // JSONEncoder serialises a dictionary rather than the order the keys were written in, so what is pinned here is the length and the absence of a space.

        let data = JsonSerializer().encodeRequest("sample.wire", [Param("a", 1), Param("b", "x")])

        XCTAssertFalse(data.contains(" "))
        XCTAssertEqual(#"{"f":"sample.wire","p":[{"n":"a","v":1},{"n":"b","v":"x"}]}"#.count, data.count)
    }

    func testRequestWithoutParamsCarriesAnEmptyArray() throws {
        // A request with no parameters carries an empty array, never null and never absent.

        let data = JsonSerializer().encodeRequest("sample.ping", [])
        let json = try XCTUnwrap(
            JSONSerialization.jsonObject(with: Data(data.utf8)) as? [String: Any]
        )

        let params = try XCTUnwrap(json["p"] as? [[String: Any]])
        XCTAssertTrue(params.isEmpty)
    }

    func testInvalidDataAnswersEmpty() {
        // Every bridge answers the empty value for its type when the data cannot be read.

        let serializer = JsonSerializer()

        XCTAssertNil(serializer.decodeRequest("not-a-json"))
        XCTAssertEqual("", serializer.decodeRequest("{}")?.functionName)

        let value: String? = serializer.decodeFunctionReturnValue("not-a-json")
        XCTAssertNil(value)

        // The empty string is what every failing path answers with, so it is the documented answer rather than a document that failed to parse.
        let empty: String? = serializer.decodeFunctionReturnValue("")
        XCTAssertNil(empty)
    }

    func testANumberThatIsNotFiniteIsRefused() {
        // The wire has no token for infinity, so writing one would put a document on it that no other bridge can read.

        let serializer = JsonSerializer()

        XCTAssertEqual("", serializer.encodeFunctionReturnValue(AnyCodable(Double.infinity)))
        XCTAssertEqual("", serializer.encodeFunctionReturnValue(AnyCodable(Double.nan)))
        XCTAssertEqual("{\"r\":2.5}", serializer.encodeFunctionReturnValue(AnyCodable(2.5)))
    }
}
