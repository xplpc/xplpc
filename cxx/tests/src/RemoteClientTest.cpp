#include "fixtures/GeneralTest.hpp"
#include "xplpc/xplpc.hpp"
#include "gtest/gtest.h"
#include <optional>

using namespace xplpc::core;
using namespace xplpc::client;
using namespace xplpc::message;

TEST_F(GeneralTest, RemoteClientTestBatteryLevel)
{
    auto request = Request{
        "platform.battery.level",
        Param<std::string>{"suffix", "%"},
    };

    auto ret = RemoteClient::call<std::string>(request);

    EXPECT_EQ(std::nullopt, ret);
}

TEST_F(GeneralTest, RemoteClientTestBatteryLevelAsync)
{
    auto request = Request{
        "platform.battery.level",
        Param<std::string>{"suffix", "%"},
    };

    auto ret = RemoteClient::callAsync<std::string>(request).get();

    EXPECT_EQ(std::nullopt, ret);
}
