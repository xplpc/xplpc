#include "fixtures/GeneralTest.hpp"
#include "xplpc/xplpc.hpp"
#include "gtest/gtest.h"

#include <optional>
#include <thread>

using namespace xplpc::core;
using namespace xplpc::data;
using namespace xplpc::client;
using namespace xplpc::message;

TEST_F(GeneralTest, RemoteClientTestBatteryLevel)
{
    auto request = Request{
        "platform.battery.level",
        Param<std::string>{"suffix", "%"},
    };

    // clang-format off
    RemoteClient::call<std::string>(request, [](const auto &response) {
        EXPECT_EQ(std::nullopt, response);
    });
    // clang-format on
}

TEST_F(GeneralTest, RemoteClientTestBatteryLevelAsync)
{
    auto request = Request{
        "platform.battery.level",
        Param<std::string>{"suffix", "%"},
    };

    // clang-format off
    std::thread([=] {
        RemoteClient::call<std::string>(request, [](const auto &response) {
            EXPECT_EQ(std::nullopt, response);
        });
    }).join();
    // clang-format on
}

TEST_F(GeneralTest, RemoteClientTestNotFound)
{
    auto request = Request{"not.found"};

    // clang-format off
    RemoteClient::call<std::string>(request, [](const auto &response) {
        EXPECT_EQ(std::nullopt, response);
    });
    // clang-format on
}
