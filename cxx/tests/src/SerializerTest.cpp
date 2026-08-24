#include "fixtures/GeneralTest.hpp"
#include "fixtures/LogRecorder.hpp"
#include "fixtures/VerifiedCall.hpp"
#include "xplpc/xplpc.hpp"
#include "gtest/gtest.h"

#include <chrono>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>

using namespace xplpc::client;
using namespace xplpc::message;
using namespace xplpc::serializer;
using namespace xplpc::proxy;
using namespace xplpc::type;

TEST_F(GeneralTest, SerializerDecodeFunctionNameFromInvalidData)
{
    EXPECT_EQ("", Serializer::decodeFunctionName(""));
    EXPECT_EQ("", Serializer::decodeFunctionName("not-a-json"));
    EXPECT_EQ("", Serializer::decodeFunctionName(R"({"p":[]})"));
    EXPECT_EQ("", Serializer::decodeFunctionName(R"({"f":123})"));
    EXPECT_EQ("sample.login", Serializer::decodeFunctionName(R"({"f":"sample.login"})"));
}

TEST_F(GeneralTest, SerializerDecodeReturnValueFromInvalidData)
{
    EXPECT_FALSE(Serializer::decodeFunctionReturnValue<std::string>("").has_value());
    EXPECT_FALSE(Serializer::decodeFunctionReturnValue<std::string>("not-a-json").has_value());
    EXPECT_FALSE(Serializer::decodeFunctionReturnValue<std::string>(R"({})").has_value());
    EXPECT_FALSE(Serializer::decodeFunctionReturnValue<std::string>(R"({"r":null})").has_value());
    EXPECT_FALSE(Serializer::decodeFunctionReturnValue<int>(R"({"r":"text"})").has_value());
    EXPECT_EQ("value", Serializer::decodeFunctionReturnValue<std::string>(R"({"r":"value"})").value());
}

TEST_F(GeneralTest, SerializerEncodeRequestWritesTheExactShape)
{
    auto data = Serializer::encodeRequest("sample.echo", Param<std::string>{"value", "text"});
    EXPECT_EQ(R"({"f":"sample.echo","p":[{"n":"value","v":"text"}]})", data);
}

TEST_F(GeneralTest, SerializerEncodeRequestWritesTheSameBytesAsEveryOtherBridge)
{
    // The five other bridges pin this exact request, so the reference has to be in that comparison rather than beside it.

    auto data = Serializer::encodeRequest("sample.wire", Param<int64_t>{"a", 1}, Param<std::string>{"b", "x"});
    EXPECT_EQ(R"({"f":"sample.wire","p":[{"n":"a","v":1},{"n":"b","v":"x"}]})", data);
}

TEST_F(GeneralTest, SerializerEncodeRequestWritesAnEmptyArrayWithNoParams)
{
    auto data = Serializer::encodeRequest("sample.ping");
    EXPECT_EQ(R"({"f":"sample.ping","p":[]})", data);
}

TEST_F(GeneralTest, SerializerEncodeRequestKeepsNullParam)
{
    auto data = Serializer::encodeRequest("sample.echo", Param<std::string>{"value", std::nullopt});
    EXPECT_EQ(R"({"f":"sample.echo","p":[{"n":"value","v":null}]})", data);
}

TEST_F(GeneralTest, ClientAnswersEmptyForMalformedRequest)
{
    auto response = std::string{"unset"};

    // clang-format off
    Client::call("not-a-json", [&response](const auto &value) {
        response = value;
    });
    // clang-format on

    EXPECT_EQ("", response);
}

TEST_F(GeneralTest, ClientAnswersEmptyForRequestWithoutFunctionName)
{
    auto response = std::string{"unset"};

    // clang-format off
    Client::call(R"({"p":[]})", [&response](const auto &value) {
        response = value;
    });
    // clang-format on

    EXPECT_EQ("", response);
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

TEST_F(GeneralTest, ClientSkipsParamWithWrongType)
{
    // The parameter is dropped instead of aborting the call, so the mapping answers with its own empty value.
    auto request = R"({"f":"sample.echo","p":[{"n":"value","v":123}]})";
    auto response = std::string{"unset"};

    // clang-format off
    Client::call(request, [&response](const auto &value) {
        response = value;
    });
    // clang-format on

    EXPECT_EQ(R"({"r":"<EMPTY>"})", response);
}

TEST_F(GeneralTest, SerializerDecodeDataViewFromInvalidData)
{
    EXPECT_FALSE(Serializer::decodeFunctionReturnValue<DataView>(R"({"r":null})").has_value());

    auto missingFields = Serializer::decodeFunctionReturnValue<DataView>(R"({"r":{}})");

    EXPECT_TRUE(missingFields.has_value());
    EXPECT_EQ(nullptr, missingFields.value().ptr());
    EXPECT_EQ(0, missingFields.value().size());
}

TEST_F(GeneralTest, SerializerEncodesTheSharedWireFormat)
{
    uint8_t buffer[] = {1, 2, 3, 4};

    auto data = Serializer::encodeRequest(
        "sample.wire",
        Param<char>{"char", 'z'},
        Param<std::chrono::system_clock::time_point>{"date", std::chrono::system_clock::time_point{std::chrono::milliseconds{494938800}}},
        Param<int64_t>{"int64", 9007199254740993},
        Param<DataView>{"dataView", DataView{buffer, sizeof(buffer)}});

    auto json = nlohmann::json::parse(data);
    auto params = json["p"];

    EXPECT_EQ(122, params[0]["v"].get<int>());
    EXPECT_TRUE(params[0]["v"].is_number());

    EXPECT_EQ(494938800, params[1]["v"].get<int64_t>());
    EXPECT_TRUE(params[1]["v"].is_number_integer());

    EXPECT_EQ(9007199254740993, params[2]["v"].get<int64_t>());

    EXPECT_TRUE(params[3]["v"].contains("ptr"));
    EXPECT_EQ(4, params[3]["v"]["size"].get<size_t>());
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

    MappingList::shared()->add("test.not.finite", Map::create<double>({}, [](const Message &, const Response &r)
                                                                      { r(std::numeric_limits<double>::infinity()); }));

    VerifiedCall::run<double>(Request{"test.not.finite"}, [](const auto &response)
                              { EXPECT_EQ(std::nullopt, response); });
}

TEST_F(GeneralTest, ProxyReportsOneLineForARequestItCannotRead)
{
    // One event is one line, so the decoder says what failed and the proxy answers without repeating it.

    NativePlatformProxy proxy;

    for (const auto &data : {"not-a-json", "null", "{\"p\":[]}"})
    {
        LogRecorder recorder;

        proxy.callProxy("test-unreadable", data);

        EXPECT_EQ(1u, recorder.errors().size()) << data;
    }
}
