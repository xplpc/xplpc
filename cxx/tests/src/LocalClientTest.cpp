#include "fixtures/GeneralTest.hpp"
#include "xplpc/xplpc.hpp"
#include "gtest/gtest.h"

#include <optional>
#include <thread>

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

    // clang-format off
    LocalClient::call<std::string>(request, [](const auto &response) {
        EXPECT_EQ("LOGGED-WITH-REMEMBER", response);
    });
    // clang-format on
}

TEST_F(GeneralTest, LocalClientTestLoginAsync)
{
    auto request = Request{
        "sample.login",
        Param<std::string>{"username", "paulo"},
        Param<std::string>{"password", "123456"},
        Param<bool>{"remember", true},
    };

    // clang-format off
    std::thread([=] {
        LocalClient::call<std::string>(request, [](const auto &response) {
            EXPECT_EQ("LOGGED-WITH-REMEMBER", response);
        });
    }).join();
    // clang-format on
}

TEST_F(GeneralTest, LocalClientTestAsync)
{
    auto request = Request{"sample.async"};

    // clang-format off
    LocalClient::call<std::string>(request, [](const auto &response) {
        EXPECT_EQ(std::nullopt, response);
    });
    // clang-format on
}

TEST_F(GeneralTest, LocalClientTestAsyncWithThread)
{
    auto request = Request{"sample.async"};

    // clang-format off
    std::thread([=] {
        LocalClient::call<std::string>(request, [](const auto &response) {
            EXPECT_EQ(std::nullopt, response);
        });
    }).join();
    // clang-format on
}

TEST_F(GeneralTest, LocalClientTestReverse)
{
    auto request = Request{"sample.reverse"};

    // clang-format off
    LocalClient::call<std::string>(request, [](const auto &response) {
        EXPECT_EQ("response-is-empty", response);
    });
    // clang-format on
}

TEST_F(GeneralTest, LocalClientTestReverseAsyncWithThread)
{
    auto request = Request{"sample.reverse"};

    // clang-format off
    std::thread([=] {
        LocalClient::call<std::string>(request, [](const auto &response) {
            EXPECT_EQ("response-is-empty", response);
        });
    }).join();
    // clang-format on
}

TEST_F(GeneralTest, LocalClientTestNotFound)
{
    auto request = Request{"not.found"};

    // clang-format off
    LocalClient::call<std::string>(request, [](const auto &response) {
        EXPECT_EQ(std::nullopt, response);
    });
    // clang-format on
}
