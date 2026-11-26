#include "fixtures/GeneralTest.hpp"
#include "fixtures/LogRecorder.hpp"
#include "fixtures/RaisingAnswer.hpp"
#include "fixtures/VerifiedCall.hpp"
#include "xplpc/xplpc.hpp"
#include "gtest/gtest.h"

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <list>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

using namespace xplpc::client;
using namespace xplpc::data;
using namespace xplpc::map;
using namespace xplpc::message;
using namespace xplpc::serializer;
using namespace xplpc::type;

TEST_F(GeneralTest, SerializerEncodeRequestKeepsNullParam)
{
    auto data = Serializer::encodeRequest("sample.echo", Param<std::string>{"value", std::nullopt});
    EXPECT_EQ(R"({"f":"sample.echo","p":[{"n":"value","v":null}]})", data);
}

TEST_F(GeneralTest, ClientAnswersNullForRequestWithMissingParams)
{
    auto request = R"({"f":"sample.login","p":[]})";
    auto response = std::string{"unset"};

    // clang-format off
    Client::call(request, [&response](const auto &value) {
        response = value;
    });
    // clang-format on

    EXPECT_EQ(R"({"r":"NOT-LOGGED"})", response);
}

TEST_F(GeneralTest, SerializerDecodeDataViewFromInvalidData)
{
    EXPECT_FALSE(Serializer::decodeFunctionReturnValue<DataView>(R"({"r":null})").has_value());

    auto missingFields = Serializer::decodeFunctionReturnValue<DataView>(R"({"r":{}})");

    EXPECT_TRUE(missingFields.has_value());
    EXPECT_EQ(0u, missingFields.value().handle());
}

TEST_F(GeneralTest, SerializerEncodesTheSharedWireFormat)
{
    uint8_t bytes[] = {1, 2, 3, 4};
    Buffer buffer{bytes, sizeof(bytes)};

    auto data = Serializer::encodeRequest(
        "sample.wire",
        Param<char>{"char", 'z'},
        Param<std::chrono::system_clock::time_point>{"date", std::chrono::system_clock::time_point{std::chrono::milliseconds{494938800}}},
        Param<int64_t>{"int64", 9007199254740993},
        Param<DataView>{"dataView", buffer.view()});

    auto json = nlohmann::json::parse(data);
    auto params = json["p"];

    EXPECT_EQ(122, params[0]["v"].get<int>());
    EXPECT_TRUE(params[0]["v"].is_number());

    EXPECT_EQ(494938800, params[1]["v"].get<int64_t>());
    EXPECT_TRUE(params[1]["v"].is_number_integer());

    EXPECT_EQ(9007199254740993, params[2]["v"].get<int64_t>());

    // A view crosses as the handle of a registered buffer and nothing else, so no address ever reaches the wire.
    EXPECT_EQ(nlohmann::json({{"buffer", buffer.view().handle()}}), params[3]["v"]);
}

TEST_F(GeneralTest, SerializerReportsNothingForAnEmptyResponse)
{
    // The empty string is what every failing path answers with, so handing it to the parser would report a failure that did not happen.

    LogRecorder recorder;

    EXPECT_EQ(std::nullopt, Serializer::decodeFunctionReturnValue<std::string>(""));
    EXPECT_TRUE(recorder.reportedNothing());

    EXPECT_EQ(std::nullopt, Serializer::decodeFunctionReturnValue<std::string>("not-a-json"));
    EXPECT_FALSE(recorder.reportedNothing());
}

TEST_F(GeneralTest, SerializerReportsNothingForAParameterCarryingNoValue)
{
    // The format lets a parameter keep its name and carry null, so reading one is not a failure to report.

    LogRecorder recorder;

    auto request = R"({"f":"sample.echo","p":[{"n":"value","v":null}]})";

    // clang-format off
    VerifiedCall::run(request, [](auto const &response) {
        EXPECT_EQ("{\"r\":\"<EMPTY>\"}", response);
    });
    // clang-format on

    EXPECT_TRUE(recorder.reportedNothing()) << (recorder.errors().empty() ? "" : recorder.errors().front());
}

TEST_F(GeneralTest, SerializerAnswersTheEmptyValueForANumberThatIsNotFinite)
{
    // The wire has no token for infinity, so a value that cannot travel reaches the caller as the empty one.

    // clang-format off
    MappingList::shared()->add("test.not.finite", Map::create<double>({}, [](const Message &, const Response &r) {
        r(std::numeric_limits<double>::infinity());
    }));

    VerifiedCall::run<double>(Request{"test.not.finite"}, [](const auto &response) {
        EXPECT_EQ(std::nullopt, response);
    });
    // clang-format on
}

TEST_F(GeneralTest, NativeLayerReportsOneLineForARequestItCannotRead)
{
    // One event is one line, so the executor that failed to read the document says so and nothing above it repeats it.

    for (const auto &data : {"not-a-json", "{\"f\":\"sample.echo\",\"p\":["})
    {
        LogRecorder recorder;

        // The name is handed over already resolved, which is how a host reaches a c++ mapping without the router scanning the document.
        const auto result = Router::shared().call(Router::shared().native(), "sample.echo", data, nullptr);

        EXPECT_TRUE(result.isAnswered()) << data;
        EXPECT_EQ("", result.value()) << data;
        EXPECT_EQ(1u, recorder.errors().size()) << data;
    }
}

TEST_F(GeneralTest, SerializerReadsAParameterOnlyAsTheTypeItIs)
{
    // A boolean is not a number, a fraction is not an integer and a value that does not fit is not the value that was sent.

    // clang-format off
    MappingList::shared()->add("test.int8", Map::create<std::string, int8_t>({"value"}, [](const Message &m, const Response &r) {
        const auto value = m.get<int8_t>("value");
        r(value ? std::to_string(value.value()) : std::string("absent"));
    }));
    // clang-format on

    const std::vector<std::pair<std::string, std::string>> cases = {
        {"-128", R"({"r":"-128"})"},
        {"127", R"({"r":"127"})"},
        {"0", R"({"r":"0"})"},
        {"128", R"({"r":"absent"})"},
        {"-129", R"({"r":"absent"})"},
        {"300", R"({"r":"absent"})"},
        {"2.5", R"({"r":"absent"})"},
        {"true", R"({"r":"absent"})"},
        {"\"1\"", R"({"r":"absent"})"},
    };

    for (const auto &[value, expected] : cases)
    {
        // clang-format off
        VerifiedCall::run(R"({"f":"test.int8","p":[{"n":"value","v":)" + value + "}]}", [&expected, &value](const std::string &response) {
            EXPECT_EQ(expected, response) << value;
        });
        // clang-format on
    }
}

TEST_F(GeneralTest, SerializerReadsAReturnValueOnlyAsTheTypeItIs)
{
    EXPECT_EQ(std::nullopt, Serializer::decodeFunctionReturnValue<int8_t>(R"({"r":300})"));
    EXPECT_EQ(std::nullopt, Serializer::decodeFunctionReturnValue<uint64_t>(R"({"r":-1})"));
    EXPECT_EQ(std::nullopt, Serializer::decodeFunctionReturnValue<std::vector<uint8_t>>(R"({"r":[1,300]})"));
    EXPECT_EQ(std::nullopt, Serializer::decodeFunctionReturnValue<std::vector<uint8_t>>(R"({"r":[1,true]})"));

    EXPECT_EQ(std::numeric_limits<uint64_t>::max(), Serializer::decodeFunctionReturnValue<uint64_t>(R"({"r":18446744073709551615})"));
    EXPECT_EQ(std::vector<uint8_t>({0, 255}), Serializer::decodeFunctionReturnValue<std::vector<uint8_t>>(R"({"r":[0,255]})"));
}

TEST_F(GeneralTest, SerializerReadsEveryStandardContainerAndEnumerationOnlyAsTheTypeItIs)
{
    // A container or an enumeration nlohmann fills on its own wraps, truncates and reads a boolean as a number unless each value inside it is checked like one at the top.

    EXPECT_EQ(std::nullopt, (Serializer::decodeFunctionReturnValue<std::list<int8_t>>(R"({"r":[300]})")));
    EXPECT_EQ(std::nullopt, (Serializer::decodeFunctionReturnValue<std::list<int8_t>>(R"({"r":[true]})")));
    EXPECT_EQ(std::nullopt, (Serializer::decodeFunctionReturnValue<std::set<uint8_t>>(R"({"r":[-1]})")));
    EXPECT_EQ(std::nullopt, (Serializer::decodeFunctionReturnValue<std::array<int, 1>>(R"({"r":[2.9]})")));
    EXPECT_EQ(std::nullopt, (Serializer::decodeFunctionReturnValue<std::array<int, 1>>(R"({"r":[1,2]})")));
    EXPECT_EQ(std::nullopt, (Serializer::decodeFunctionReturnValue<std::pair<int, bool>>(R"({"r":[1,1]})")));
    EXPECT_EQ(std::nullopt, (Serializer::decodeFunctionReturnValue<std::tuple<int8_t, std::string>>(R"({"r":[300,"a"]})")));
    EXPECT_EQ(std::nullopt, (Serializer::decodeFunctionReturnValue<std::map<int, int8_t>>(R"({"r":[[1,300]]})")));
    EXPECT_EQ(std::nullopt, (Serializer::decodeFunctionReturnValue<std::byte>(R"({"r":300})")));
    EXPECT_EQ(std::nullopt, (Serializer::decodeFunctionReturnValue<std::byte>(R"({"r":true})")));

    EXPECT_EQ(std::list<int8_t>({1, -2}), (Serializer::decodeFunctionReturnValue<std::list<int8_t>>(R"({"r":[1,-2]})")));
    EXPECT_EQ(std::set<uint8_t>({255}), (Serializer::decodeFunctionReturnValue<std::set<uint8_t>>(R"({"r":[255]})")));
    EXPECT_EQ((std::array<int, 1>{3}), (Serializer::decodeFunctionReturnValue<std::array<int, 1>>(R"({"r":[3]})")));
    EXPECT_EQ((std::pair<int, bool>{1, true}), (Serializer::decodeFunctionReturnValue<std::pair<int, bool>>(R"({"r":[1,true]})")));
    EXPECT_EQ((std::tuple<int8_t, std::string>{-1, "a"}), (Serializer::decodeFunctionReturnValue<std::tuple<int8_t, std::string>>(R"({"r":[-1,"a"]})")));
    EXPECT_EQ((std::map<int, int8_t>{{1, 2}}), (Serializer::decodeFunctionReturnValue<std::map<int, int8_t>>(R"({"r":[[1,2]]})")));
    EXPECT_EQ(std::byte{7}, Serializer::decodeFunctionReturnValue<std::byte>(R"({"r":7})"));
}

TEST_F(GeneralTest, SerializerDecodesNoBufferFromAViewThatIsNotAHandle)
{
    // A handle is an unsigned integer, so a boolean, a fraction or a negative number names no buffer at all.

    for (const auto *document : {R"({"r":{"buffer":true}})", R"({"r":{"buffer":1.5}})", R"({"r":{"buffer":-8}})", R"({"r":{}})"})
    {
        LogRecorder recorder;

        auto view = Serializer::decodeFunctionReturnValue<DataView>(document);

        ASSERT_TRUE(view.has_value()) << document;
        EXPECT_EQ(0u, view.value().handle()) << document;
        EXPECT_FALSE(view.value().bytes().has_value()) << document;

        // Every bridge says so rather than answering a view that names nothing in silence.
        ASSERT_EQ(1u, recorder.errors().size()) << document;
        EXPECT_NE(std::string::npos, recorder.errors().front().find("Value is not a data view")) << document;
    }
}

TEST_F(GeneralTest, SerializerReportsAnAnswerOfAnotherTypeAsOneLine)
{
    // A document that parsed and carries a value the type cannot hold is not a parse failure, and every bridge says the same thing about it.

    LogRecorder recorder;

    EXPECT_EQ(std::nullopt, Serializer::decodeFunctionReturnValue<int>(R"({"r":"text"})"));

    ASSERT_EQ(1u, recorder.errors().size());
    EXPECT_NE(std::string::npos, recorder.errors().front().find("Value has another type"));
}

TEST_F(GeneralTest, SerializerReportsNothingForAnAnswerWhoseRootIsNotAnObject)
{
    // A document that carries no value answers the empty one, which is what every bridge reads such a root as.

    LogRecorder recorder;

    EXPECT_EQ(std::nullopt, Serializer::decodeFunctionReturnValue<int>("[1]"));
    EXPECT_EQ(std::nullopt, Serializer::decodeFunctionReturnValue<int>("42"));

    EXPECT_TRUE(recorder.reportedNothing()) << (recorder.errors().empty() ? "" : recorder.errors().front());
}

TEST_F(GeneralTest, SerializerAnswersNothingForAConsumerTypeRaisingSomethingThatIsNotAnException)
{
    // The decode runs outside every guard a caller has, so whatever a consumer type raises has to stop here.

    LogRecorder recorder;

    EXPECT_FALSE(Serializer::decodeFunctionReturnValue<RaisingAnswer>(R"({"r":{}})").has_value());
    EXPECT_EQ(1u, recorder.errors().size());
}

TEST_F(GeneralTest, SerializerAnswersTheCallerWhenTheRequestCannotBeEncoded)
{
    // Text that is not utf8 cannot be written as json, which is reported once and answered rather than raised out of the request.

    LogRecorder recorder;

    const Request request{"sample.echo", Param<std::string>{"value", std::string("\xff\xfe")}};

    EXPECT_EQ("", request.data());
    EXPECT_EQ(std::nullopt, Client::callSync<std::string>(request));

    ASSERT_EQ(1u, recorder.errors().size());
    EXPECT_NE(std::string::npos, recorder.errors().front().find("Error when encode data"));
}

TEST_F(GeneralTest, SerializerNamesAParameterOfAnotherTypeAsEveryBridgeDoes)
{
    LogRecorder recorder;

    // clang-format off
    VerifiedCall::run(R"({"f":"sample.echo","p":[{"n":"value","v":true}]})", [](const std::string &response) {
        EXPECT_EQ(R"({"r":"<EMPTY>"})", response);
    });
    // clang-format on

    ASSERT_EQ(1u, recorder.errors().size());
    EXPECT_NE(std::string::npos, recorder.errors().front().find("Value of \"value\" has another type"));
}

TEST_F(GeneralTest, SerializerSaysARequestNamesNoFunctionAsEveryBridgeDoes)
{
    LogRecorder recorder;

    EXPECT_EQ("", Serializer::decodeFunctionName(R"({"p":[]})"));

    ASSERT_EQ(1u, recorder.errors().size());
    EXPECT_NE(std::string::npos, recorder.errors().front().find("The request names no function"));
}

TEST_F(GeneralTest, SerializerReadsAFloatingValueOnlyWhenTheTypeHoldsIt)
{
    // A magnitude past the type would arrive as infinity and an integer past its precision would arrive as another integer.

    EXPECT_EQ(std::nullopt, Serializer::decodeFunctionReturnValue<float>(R"({"r":-1e300})"));
    EXPECT_EQ(std::nullopt, Serializer::decodeFunctionReturnValue<float>(R"({"r":16777217})"));
    EXPECT_EQ(std::nullopt, Serializer::decodeFunctionReturnValue<double>(R"({"r":9007199254740993})"));
    EXPECT_EQ(std::nullopt, Serializer::decodeFunctionReturnValue<double>(R"({"r":18446744073709551615})"));
    EXPECT_EQ(std::nullopt, Serializer::decodeFunctionReturnValue<std::vector<float>>(R"({"r":[1,1e300]})"));

    EXPECT_EQ(std::numeric_limits<float>::max(), Serializer::decodeFunctionReturnValue<float>(R"({"r":3.4028234663852886e38})"));
    EXPECT_EQ(16777216.0f, Serializer::decodeFunctionReturnValue<float>(R"({"r":16777216})"));
    EXPECT_EQ(2.5f, Serializer::decodeFunctionReturnValue<float>(R"({"r":2.5})"));
    EXPECT_EQ(1e300, Serializer::decodeFunctionReturnValue<double>(R"({"r":1e300})"));
    EXPECT_EQ(9007199254740992.0, Serializer::decodeFunctionReturnValue<double>(R"({"r":9007199254740992})"));
    EXPECT_EQ(-9223372036854775808.0, Serializer::decodeFunctionReturnValue<double>(R"({"r":-9223372036854775808})"));
}

TEST_F(GeneralTest, SerializerReadsACharOnlyAsACodePointEveryAbiHolds)
{
    // A char is signed on x86 and apple arm64 and unsigned on android and linux arm64, so a code point is read only where both agree.

    EXPECT_EQ(std::optional<char>{'z'}, Serializer::decodeFunctionReturnValue<char>(R"({"r":122})"));
    EXPECT_EQ(std::nullopt, Serializer::decodeFunctionReturnValue<char>(R"({"r":-1})"));
    EXPECT_EQ(std::nullopt, Serializer::decodeFunctionReturnValue<char>(R"({"r":233})"));
}

TEST_F(GeneralTest, SerializerReadsATimestampOnlyWhenTheClockHoldsIt)
{
    // A clock counting nanoseconds ends in the year 2262, so a later date is refused rather than overflowing into another one.

    using Nanoseconds = std::chrono::time_point<std::chrono::system_clock, std::chrono::nanoseconds>;
    using Seconds = std::chrono::time_point<std::chrono::system_clock, std::chrono::seconds>;

    // A failing comparison prints a time point through a formatter libc++ ships only from macos 13.3, so what is compared is the count it holds.
    // clang-format off
    const auto ticks = [](const auto &decoded) -> std::optional<long long> {
        if (!decoded)
        {
            return std::nullopt;
        }

        return decoded->time_since_epoch().count();
    };
    // clang-format on

    EXPECT_EQ(std::nullopt, ticks(Serializer::decodeFunctionReturnValue<Nanoseconds>(R"({"r":253402300799000})")));
    EXPECT_EQ(std::nullopt, ticks(Serializer::decodeFunctionReturnValue<Nanoseconds>(R"({"r":9223372036855})")));
    EXPECT_EQ(std::nullopt, ticks(Serializer::decodeFunctionReturnValue<Nanoseconds>(R"({"r":-9223372036855})")));

    EXPECT_EQ(9223372036854000000LL, ticks(Serializer::decodeFunctionReturnValue<Nanoseconds>(R"({"r":9223372036854})")));
    EXPECT_EQ(-9223372036854000000LL, ticks(Serializer::decodeFunctionReturnValue<Nanoseconds>(R"({"r":-9223372036854})")));
    EXPECT_EQ(253402300799LL, ticks(Serializer::decodeFunctionReturnValue<Seconds>(R"({"r":253402300799000})")));

    // A consumer type fills its timestamp through its own from_json, which does not pass through the guard.
    EXPECT_THROW(nlohmann::json(253402300799000).get<Nanoseconds>(), std::out_of_range);
}

TEST_F(GeneralTest, SerializerWritesNullForAMappingAnsweringNullptr)
{
    // A mapping declared to answer an object answers nullptr when it has nothing, and that is the null the format carries rather than a failed encode.

    LogRecorder recorder;

    // clang-format off
    VerifiedCall::run(R"({"f":"sample.todo.single","p":[{"n":"item","v":null}]})", [](const std::string &response) {
        EXPECT_EQ(R"({"r":null})", response);
    });
    // clang-format on

    EXPECT_TRUE(recorder.reportedNothing()) << (recorder.errors().empty() ? "" : recorder.errors().front());
}
