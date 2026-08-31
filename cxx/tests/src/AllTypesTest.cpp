#include "xplpc/custom/AllTypes.hpp"
#include "fixtures/GeneralTest.hpp"
#include "fixtures/VerifiedCall.hpp"
#include "xplpc/xplpc.hpp"
#include "gtest/gtest.h"

#include <chrono>
#include <limits>
#include <optional>
#include <thread>
#include <vector>

using namespace xplpc::core;
using namespace xplpc::client;
using namespace xplpc::message;

AllTypes createItem()
{
    // The edges are what a serializer loses, since a small value round trips even when the format cannot carry the type.
    return AllTypes{
        std::numeric_limits<int8_t>::min(),
        std::numeric_limits<int16_t>::min(),
        std::numeric_limits<int32_t>::min(),
        9007199254740993,
        0.1f,
        0.1,
        true,
        std::nullopt,
        {},
        {},
        std::chrono::system_clock::time_point{std::chrono::milliseconds(494938800)},
        'z',
        0x00AE,
        0x00AE,
        0x00AE,
        "ok",
    };
}

void validateAllTypes(const AllTypes &allTypes)
{
    auto mapValue = allTypes.typeMap.at("item1");
    auto timestamp = std::chrono::time_point_cast<std::chrono::milliseconds>(allTypes.typeDateTime).time_since_epoch().count();

    EXPECT_EQ(std::numeric_limits<int8_t>::min(), allTypes.typeInt8);
    EXPECT_EQ(std::numeric_limits<int16_t>::min(), allTypes.typeInt16);
    EXPECT_EQ(std::numeric_limits<int32_t>::min(), allTypes.typeInt32);
    EXPECT_EQ(9007199254740993, allTypes.typeInt64);
    EXPECT_EQ(0.1f, allTypes.typeFloat32);
    EXPECT_EQ(0.1, allTypes.typeFloat64);
    EXPECT_EQ(true, allTypes.typeBool);
    EXPECT_EQ(std::nullopt, allTypes.typeOptional);
    EXPECT_EQ(1, allTypes.typeList.size());
    EXPECT_EQ("ok", mapValue);
    EXPECT_EQ(494938800, timestamp);
    EXPECT_EQ('z', allTypes.typeChar);
    EXPECT_EQ(0x00AE, allTypes.typeChar16);
    EXPECT_EQ(0x00AE, allTypes.typeChar32);
    EXPECT_EQ(0x00AE, allTypes.typeWchar);
    EXPECT_EQ("ok", allTypes.typeString);
}

TEST_F(GeneralTest, AllTypesTestSingle)
{
    auto item = createItem();
    item.typeMap["item1"] = "ok";
    item.typeList.push_back(item);

    auto request = Request{"sample.alltypes.single", Param<AllTypes>{"item", item}};

    // clang-format off
    VerifiedCall::run<AllTypes>(request, [](const auto &response) {
        EXPECT_NE(response, std::nullopt);

        if (response)
        {
            auto allTypes = response.value();
            validateAllTypes(allTypes);
        }
    });
    // clang-format on
}

TEST_F(GeneralTest, AllTypesTestSingleAsync)
{
    auto item = createItem();
    item.typeMap["item1"] = "ok";
    item.typeList.push_back(item);

    auto request = Request{"sample.alltypes.single", Param<AllTypes>{"item", item}};

    // clang-format off
    std::thread([=] {
        VerifiedCall::run<AllTypes>(request, [](const auto &response) {
            EXPECT_NE(response, std::nullopt);

            if (response)
            {
                auto allTypes = response.value();
                validateAllTypes(allTypes);
            }
        });
    }).join();
    // clang-format on
}

TEST_F(GeneralTest, AllTypesMultipleTest)
{
    auto item1 = createItem();
    item1.typeMap["item1"] = "xyz";

    auto item2 = createItem();
    item1.typeMap["item2"] = "xyz";

    auto items = std::vector<AllTypes>{item1, item2};

    auto request = Request{"sample.alltypes.list", Param<std::vector<AllTypes>>{"items", items}};

    // clang-format off
    VerifiedCall::run<std::vector<AllTypes>>(request, [](const auto &response) {
        EXPECT_NE(response, std::nullopt);

        auto total = 0;

        if (response)
        {
            auto list = response.value();
            total = list.size();
        }

        EXPECT_EQ(2, total);
    });
    // clang-format on
}

TEST_F(GeneralTest, AllTypesMultipleTestAsync)
{
    auto item1 = createItem();
    item1.typeMap["item1"] = "xyz";

    auto item2 = createItem();
    item1.typeMap["item2"] = "xyz";

    auto items = std::vector<AllTypes>{item1, item2};

    auto request = Request{"sample.alltypes.list", Param<std::vector<AllTypes>>{"items", items}};

    // clang-format off
    std::thread([=] {
        VerifiedCall::run<std::vector<AllTypes>>(request, [](const auto &response) {
            EXPECT_NE(response, std::nullopt);

            auto total = 0;

            if (response)
            {
                auto list = response.value();
                total = list.size();
            }

            EXPECT_EQ(2, total);
        });
    }).join();
    // clang-format on
}

TEST_F(GeneralTest, AllTypesListDoesNotReadPastAListTheCallerSentShort)
{
    // How many items arrive is decided by the caller, so a mapping must not reach for ones that are not there.

    auto request = Request{"sample.alltypes.list", Param<std::vector<AllTypes>>{"items", {}}};

    // clang-format off
    VerifiedCall::run<std::vector<AllTypes>>(request, [](const auto &response) {
        ASSERT_TRUE(response.has_value());
        EXPECT_TRUE(response.value().empty());
    });
    // clang-format on
}
