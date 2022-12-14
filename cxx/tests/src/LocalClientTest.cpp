#include "fixtures/GeneralTest.hpp"
#include "xplpc/xplpc.hpp"
#include "gtest/gtest.h"

using namespace xplpc::core;
using namespace xplpc::client;
using namespace xplpc::message;

TEST_F(GeneralTest, LocalClientTestLogin)
{
    auto request = Request{
        "sample.login",
        Param<std::string>{"username", "paulo"},
        Param<std::string>{"password", "123456"},
        Param<bool>{"remember", true},
    };

    auto ret = LocalClient::call<std::string>(request).value();

    EXPECT_EQ("LOGGED-WITH-REMEMBER", ret);
}

TEST_F(GeneralTest, LocalClientTestLoginAsync)
{
    auto request = Request{
        "sample.login",
        Param<std::string>{"username", "paulo"},
        Param<std::string>{"password", "123456"},
        Param<bool>{"remember", true},
    };

    auto ret = LocalClient::callAsync<std::string>(request).get().value();

    EXPECT_EQ("LOGGED-WITH-REMEMBER", ret);
}
