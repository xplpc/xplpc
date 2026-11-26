#include "fixtures/ConformanceCases.hpp"
#include "fixtures/GeneralTest.hpp"
#include "fixtures/RecordedJson.hpp"
#include "xplpc/custom/Todo.hpp"
#include "xplpc/custom/json/TodoJson.hpp"
#include "xplpc/xplpc.hpp"
#include "gtest/gtest.h"

#include <chrono>
#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

using namespace xplpc::message;
using namespace xplpc::serializer;

class WireConformance : public GeneralTest, public ::testing::WithParamInterface<std::string>
{
protected:
    using Date = std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds>;
    using Writer = std::function<std::string(const std::string &, const std::string &, const nlohmann::json &)>;

    template <typename T>
    static std::string encodeAs(const std::string &function, const std::string &name, const T &value)
    {
        return Serializer::encodeRequest(function, Param<T>{name, value});
    }

    template <typename T>
    static std::string encodeRead(const std::string &function, const std::string &name, const nlohmann::json &value)
    {
        return encodeAs<T>(function, name, value.get<T>());
    }

    // clang-format off
    static inline const std::unordered_map<std::string, Writer> writers = {
        {"null", [](const auto &function, const auto &name, const auto &) {
            return Serializer::encodeRequest(function, Param<std::string>{name, std::nullopt});
        }},
        {"bool", encodeRead<bool>},
        {"int", encodeRead<std::int64_t>},
        {"int64", encodeRead<std::int64_t>},
        {"uint64", encodeRead<std::uint64_t>},
        {"double", encodeRead<double>},
        {"string", encodeRead<std::string>},
        {"char", encodeRead<char>},
        {"date", [](const auto &function, const auto &name, const auto &value) {
            return encodeAs(function, name, Date{std::chrono::milliseconds{value.template get<std::int64_t>()}});
        }},
        {"dataview", [](const auto &function, const auto &name, const auto &value) {
            return encodeAs(function, name, xplpc::type::DataView{value.template get<xplpc::core::Handle>()});
        }},
        {"list<int>", encodeRead<std::vector<std::int64_t>>},
        {"list<string>", encodeRead<std::vector<std::string>>},
        {"list<list<int>>", encodeRead<std::vector<std::vector<std::int64_t>>>},
        {"list<dataview>", [](const auto &function, const auto &name, const auto &value) {
            std::vector<xplpc::type::DataView> views;

            for (const auto &handle : value)
            {
                views.emplace_back(handle.template get<xplpc::core::Handle>());
            }

            return encodeAs(function, name, views);
        }},
        {"map<int>", encodeRead<std::map<std::string, std::int64_t>>},
        {"record", [](const auto &function, const auto &name, const auto &value) {
            return encodeAs(function, name, Recorded{value["count"].template get<std::int64_t>(), value["name"].template get<std::string>()});
        }},
    };
    // clang-format on
};

class AcceptConformance : public GeneralTest, public ::testing::WithParamInterface<std::string>
{
};

class RefuseConformance : public GeneralTest, public ::testing::WithParamInterface<std::string>
{
};

class ValuesConformance : public GeneralTest, public ::testing::WithParamInterface<std::string>
{
protected:
    using Date = std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds>;

    // A value read is compared as what it writes back, which is the form the case expects it in, while a float is compared as a float since writing it widens it.
    template <typename T>
    static void expectValue(const nlohmann::json &testCase)
    {
        const auto value = Serializer::decodeFunctionReturnValue<T>(testCase["document"].get<std::string>());

        if (testCase["expect"].is_null())
        {
            EXPECT_FALSE(value.has_value()) << "read " << written(value.value()).dump();
            return;
        }

        ASSERT_TRUE(value.has_value());

        if constexpr (std::is_same_v<T, float>)
        {
            EXPECT_EQ(testCase["expect"].get<float>(), value.value());
        }
        else
        {
            EXPECT_EQ(testCase["expect"], written(value.value()));
        }
    }

    template <typename T>
    static nlohmann::json written(const T &value)
    {
        if constexpr (std::is_same_v<T, xplpc::type::DataView>)
        {
            return value.handle();
        }
        else
        {
            return value;
        }
    }

    // clang-format off
    static inline const std::unordered_map<std::string, std::function<void(const nlohmann::json &)>> readers = {
        {"bool", expectValue<bool>},
        {"int8", expectValue<std::int8_t>},
        {"int16", expectValue<std::int16_t>},
        {"int32", expectValue<std::int32_t>},
        {"int64", expectValue<std::int64_t>},
        {"uint8", expectValue<std::uint8_t>},
        {"uint16", expectValue<std::uint16_t>},
        {"uint32", expectValue<std::uint32_t>},
        {"uint64", expectValue<std::uint64_t>},
        {"float", expectValue<float>},
        {"double", expectValue<double>},
        {"string", expectValue<std::string>},
        {"char", expectValue<char>},
        {"date", expectValue<Date>},
        {"dataview", expectValue<xplpc::type::DataView>},
        {"list<int32>", expectValue<std::vector<std::int32_t>>},
        {"list<string>", expectValue<std::vector<std::string>>},
        {"list<list<int32>>", expectValue<std::vector<std::vector<std::int32_t>>>},
        {"map<int32>", expectValue<std::map<std::string, std::int32_t>>},
        {"todo", expectValue<Todo>},
    };
    // clang-format on
};

TEST_P(WireConformance, EncodesTheExactBytes)
{
    // A request of one parameter is built from the type the case declares, the way a caller builds it, while the encoder takes the types of a longer one at compile time, so those are carried as json values, which encode as their declared types would.

    const auto testCase = ConformanceCases::read("wire", GetParam());
    const auto function = testCase["function"].get<std::string>();
    const auto &declared = testCase["params"];

    std::string bytes;

    if (declared.empty())
    {
        bytes = Serializer::encodeRequest(function);
    }
    else if (declared.size() == 1)
    {
        const auto writer = writers.find(declared[0]["type"].get<std::string>());

        ASSERT_NE(writers.end(), writer) << "The case declares a parameter type this runner does not know";

        bytes = writer->second(function, declared[0]["name"].get<std::string>(), declared[0]["value"]);
    }
    else
    {
        ASSERT_EQ(2u, declared.size()) << "This runner builds requests of up to two parameters";

        bytes = Serializer::encodeRequest(function, Param<nlohmann::json>{declared[0]["name"].get<std::string>(), declared[0]["value"]}, Param<nlohmann::json>{declared[1]["name"].get<std::string>(), declared[1]["value"]});
    }

    EXPECT_EQ(testCase["bytes"].get<std::string>(), bytes);
}

TEST_P(AcceptConformance, RoutesTheFunctionItNames)
{
    // C++ decodes a parameter against the type its mapping declares, so routing reads the function and a mapping reads the rest.

    const auto testCase = ConformanceCases::read("accept", GetParam());

    EXPECT_EQ(testCase["function"].get<std::string>(), Serializer::decodeFunctionName(testCase["document"].get<std::string>()));
}

TEST_P(RefuseConformance, RoutesNothing)
{
    const auto testCase = ConformanceCases::read("refuse", GetParam());

    EXPECT_EQ("", Serializer::decodeFunctionName(testCase["document"].get<std::string>()));
}

TEST_P(ValuesConformance, ReadsTheDeclaredType)
{
    const auto testCase = ConformanceCases::read("values", GetParam());
    const auto reader = readers.find(testCase["type"].get<std::string>());

    ASSERT_NE(readers.end(), reader) << "The case declares a type this runner does not know";

    reader->second(testCase);
}

// clang-format off
const auto conformanceName = [](const ::testing::TestParamInfo<std::string> &info) {
    return ConformanceCases::testName(info.param);
};
// clang-format on

INSTANTIATE_TEST_SUITE_P(Conformance, WireConformance, ::testing::ValuesIn(ConformanceCases::in("wire")), conformanceName);
INSTANTIATE_TEST_SUITE_P(Conformance, AcceptConformance, ::testing::ValuesIn(ConformanceCases::in("accept")), conformanceName);
INSTANTIATE_TEST_SUITE_P(Conformance, RefuseConformance, ::testing::ValuesIn(ConformanceCases::in("refuse")), conformanceName);
INSTANTIATE_TEST_SUITE_P(Conformance, ValuesConformance, ::testing::ValuesIn(ConformanceCases::in("values")), conformanceName);
