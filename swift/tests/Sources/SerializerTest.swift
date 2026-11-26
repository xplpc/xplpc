import XCTest
import xplpc

final class SerializerTest: XPLPCTestCase {
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

    func testDataViewTravelsAsItsBufferHandleAlone() throws {
        // A view carries the handle its owner registered and never an address, so nothing else may appear beside it.

        let params = try encodedParams([Param("dataView", DataView(buffer: 42))])
        let value = try XCTUnwrap(params[0]["v"] as? [String: Any])

        XCTAssertEqual(["buffer"], Array(value.keys))
        XCTAssertEqual(42, value["buffer"] as? UInt64)
        XCTAssertEqual(#"{"buffer":42}"#, try String(bytes: JSONEncoder().encode(DataView(buffer: 42)), encoding: .utf8))
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

    func testANumberThatIsNotFiniteIsRefused() {
        // The wire has no token for infinity, so writing one would put a document on it that no other bridge can read.

        let serializer = JsonSerializer()

        XCTAssertEqual("", serializer.encodeFunctionReturnValue(AnyCodable(Double.infinity)))
        XCTAssertEqual("", serializer.encodeFunctionReturnValue(AnyCodable(Double.nan)))
        XCTAssertEqual("{\"r\":2.5}", serializer.encodeFunctionReturnValue(AnyCodable(2.5)))
    }

    func testADateWholeMillisecondsCannotHoldIsRefusedRatherThanTrapping() {
        let serializer = JsonSerializer()

        XCTAssertEqual("", serializer.encodeFunctionReturnValue(AnyCodable(Date(timeIntervalSince1970: .infinity))))
        XCTAssertEqual("", serializer.encodeFunctionReturnValue(AnyCodable(Date(timeIntervalSince1970: .nan))))
        XCTAssertEqual("", serializer.encodeFunctionReturnValue(AnyCodable(Date(timeIntervalSince1970: 1e17))))
        XCTAssertEqual("{\"r\":494938800}", serializer.encodeFunctionReturnValue(AnyCodable(Date(timeIntervalSince1970: 494_938.8))))
    }

    func testAByteTravelsAsTheIntegerItIs() throws {
        // A Swift Int8 bridges to the same objective-c char a boolean does, so only the CFBoolean singleton may travel as true or false.

        let params = try encodedParams([
            Param("min", Int8.min),
            Param("negative", Int8(-5)),
            Param("zero", Int8(0)),
            Param("max", Int8.max),
            Param("unsigned", UInt8.max),
            Param("yes", true),
            Param("no", false),
            Param("bridgedByte", NSNumber(value: Int8(-5))),
            Param("bridgedBool", NSNumber(value: true)),
        ])

        let values = params.map { $0["v"] as? NSNumber }

        XCTAssertEqual([-128, -5, 0, 127, 255], values[0 ..< 5].map { $0?.intValue })
        XCTAssertEqual(-5, values[7]?.intValue)

        for index in [0, 1, 2, 3, 4, 7] {
            XCTAssertNotEqual(CFBooleanGetTypeID(), CFGetTypeID(values[index]), "parameter \(index) travelled as a boolean")
        }

        for index in [5, 6, 8] {
            XCTAssertEqual(CFBooleanGetTypeID(), CFGetTypeID(values[index]), "parameter \(index) did not travel as a boolean")
        }

        XCTAssertEqual([true, false, true], [values[5], values[6], values[8]].map { $0?.boolValue })
    }

    func testANumberIsReadAsAnyNumericTypeThatHoldsItExactly() throws {
        let data = #"{"f":"platform.byte","p":[{"n":"level","v":42},{"n":"big","v":9007199254740993},{"n":"over","v":128},{"n":"flag","v":true},{"n":"fraction","v":2.5},{"n":"text","v":"42"}]}"#
        let message = try XCTUnwrap(JsonSerializer().decodeRequest(data)?.message)

        XCTAssertEqual(42, message.get("level") as Int8?)
        XCTAssertEqual(42, message.get("level") as UInt8?)
        XCTAssertEqual(42, message.get("level") as Int16?)
        XCTAssertEqual(42, message.get("level") as Int64?)
        XCTAssertEqual(42, message.get("level") as Double?)
        XCTAssertEqual(9_007_199_254_740_993, message.get("big") as Int64?)
        XCTAssertEqual(128, message.get("over") as UInt8?)
        XCTAssertEqual(2.5, message.get("fraction") as Double?)
        XCTAssertEqual(2.5, message.get("fraction") as Float?)
        XCTAssertEqual(true, message.get("flag") as Bool?)
        XCTAssertEqual("42", message.get("text") as String?)

        // A value that does not fit, a fraction read as an integer and a crossing between a boolean and a number are all refused rather than bent.
        XCTAssertNil(message.get("over") as Int8?)
        XCTAssertNil(message.get("big") as Double?)
        XCTAssertNil(message.get("fraction") as Int?)
        XCTAssertNil(message.get("flag") as Int?)
        XCTAssertNil(message.get("level") as Bool?)
        XCTAssertNil(message.get("text") as Int?)
        XCTAssertNil(message.get("level") as String?)
    }

    func testAListOrAnObjectIsReadOnlyAsATypeEveryValueInItHolds() throws {
        // A boolean inside a list or an object is not a number and a number inside one is not a boolean, on a parameter and on an answer alike.

        let data = #"{"f":"platform.list","p":[{"n":"flags","v":[true,false]},{"n":"levels","v":[1,0]},{"n":"named","v":{"on":true}}]}"#
        let message = try XCTUnwrap(JsonSerializer().decodeRequest(data)?.message)

        XCTAssertEqual([true, false], message.get("flags") as [Bool]?)
        XCTAssertEqual([1, 0], message.get("levels") as [Int]?)
        XCTAssertEqual(["on": true], message.get("named") as [String: Bool]?)

        XCTAssertNil(message.get("flags") as [Int]?)
        XCTAssertNil(message.get("levels") as [Bool]?)
        XCTAssertNil(message.get("named") as [String: Int]?)
        XCTAssertNil(JsonSerializer().decodeFunctionReturnValue(#"{"r":[true]}"#) as [Int]?)
    }

    func testANumberNestedInAValueIsReadAsExactlyAsOneAtTheTop() throws {
        // A number inside a list, an object or a type of the caller's own goes through the checks a number at the top does, so an integer a double would round and a fraction an integer type would drop are both refused.

        let data = #"{"f":"platform.nested","p":[{"n":"wide","v":[9007199254740993]},{"n":"whole","v":[1.0]},{"n":"named","v":{"a":2.5}},{"n":"counted","v":{"count":1.0,"name":"n"}}]}"#
        let message = try XCTUnwrap(JsonSerializer().decodeRequest(data)?.message)

        XCTAssertEqual([9_007_199_254_740_993], message.get("wide") as [Int64]?)
        XCTAssertEqual([1.0], message.get("whole") as [Double]?)

        XCTAssertNil(message.get("wide") as [Double]?)
        XCTAssertNil(message.get("whole") as [Int]?)
        XCTAssertNil(message.get("named") as [String: Int]?)
        XCTAssertNil(message.get("counted") as Counted?)
        XCTAssertNil(JsonSerializer().decodeFunctionReturnValue(#"{"r":{"a":[1.0]}}"#) as [String: [Int]]?)
    }

    func testADateAViewAndACodePointNestedInAValueAreStillRead() throws {
        let answer = #"{"r":{"at":1700000000123,"view":{"buffer":7},"initial":65,"note":null}}"#
        let stamped = try XCTUnwrap(JsonSerializer().decodeFunctionReturnValue(answer) as Stamped?)

        XCTAssertEqual(Date(timeIntervalSince1970: 1_700_000_000.123), stamped.at)
        XCTAssertEqual(7, stamped.view.buffer)
        XCTAssertEqual("A", stamped.initial.scalar)
        XCTAssertNil(stamped.note)
    }

    func testADecimalAUrlAndDataNestedInAValueReadBackWhatTheEncoderWrote() throws {
        let price = try XCTUnwrap(Decimal(string: "1.25"))
        let link = try XCTUnwrap(URL(string: "https://example.com/a"))
        let priced = Priced(price: price, link: link, blob: Data([1, 2]))
        let answer = JsonSerializer().encodeFunctionReturnValue(AnyCodable(priced))

        XCTAssertEqual(priced, JsonSerializer().decodeFunctionReturnValue(answer) as Priced?)
        XCTAssertEqual(price, JsonSerializer().decodeFunctionReturnValue(#"{"r":1.25}"#) as Decimal?)
        XCTAssertNil(JsonSerializer().decodeFunctionReturnValue(#"{"r":true}"#) as Decimal?)
    }

    func testATypeDecodingItsSuperclassReadsWhatTheEncoderWroteUnderSuper() throws {
        let answer = JsonSerializer().encodeFunctionReturnValue(AnyCodable(Derived(base: 1, derived: 2)))
        let derived = try XCTUnwrap(JsonSerializer().decodeFunctionReturnValue(answer) as Derived?)

        XCTAssertEqual(1, derived.base)
        XCTAssertEqual(2, derived.derived)
    }

    func testAMagnitudePastAFloatingTypeIsRefusedRatherThanReadAsInfinity() throws {
        let data = #"{"f":"platform.float","p":[{"n":"huge","v":1e300},{"n":"largest","v":3.4028234663852886e38}]}"#
        let message = try XCTUnwrap(JsonSerializer().decodeRequest(data)?.message)

        XCTAssertNil(message.get("huge") as Float?)
        XCTAssertEqual(1e300, message.get("huge") as Double?)
        XCTAssertEqual(Float.greatestFiniteMagnitude, message.get("largest") as Float?)
    }
}

private struct Counted: Decodable {
    let count: Int
    let name: String
}

private struct Stamped: Decodable {
    let at: Date
    let view: DataView
    let initial: CodePoint
    let note: String?
}

private struct Priced: Codable, Equatable {
    let price: Decimal
    let link: URL
    let blob: Data
}

private class Base: Codable {
    let base: Int

    init(base: Int) {
        self.base = base
    }
}

private final class Derived: Base {
    let derived: Int

    private enum CodingKeys: String, CodingKey {
        case derived
    }

    init(base: Int, derived: Int) {
        self.derived = derived
        super.init(base: base)
    }

    required init(from decoder: Decoder) throws {
        let container = try decoder.container(keyedBy: CodingKeys.self)
        derived = try container.decode(Int.self, forKey: .derived)
        try super.init(from: container.superDecoder())
    }

    override func encode(to encoder: Encoder) throws {
        var container = encoder.container(keyedBy: CodingKeys.self)
        try container.encode(derived, forKey: .derived)
        try super.encode(to: container.superEncoder())
    }
}
