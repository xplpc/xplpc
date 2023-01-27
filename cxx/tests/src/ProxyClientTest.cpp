#include "fixtures/GeneralTest.hpp"
#include "xplpc/xplpc.hpp"
#include "gtest/gtest.h"

#include <thread>

using namespace xplpc::core;
using namespace xplpc::client;

TEST_F(GeneralTest, ProxyClientTestLogin)
{
    auto request = R"({"f":"sample.login","p":[{"n":"username","v":"paulo"},{"n":"password","v":"123456"},{"n":"remember","v":true}]})";

    // clang-format off
    ProxyClient::call(request, [](auto const &response) {
        EXPECT_EQ("{\"r\":\"LOGGED-WITH-REMEMBER\"}", response);
    });
    // clang-format on
}

TEST_F(GeneralTest, ProxyClientTestLoginAsync)
{
    auto request = R"({"f":"sample.login","p":[{"n":"username","v":"paulo"},{"n":"password","v":"123456"},{"n":"remember","v":true}]})";

    // clang-format off
    std::thread([=] {
        ProxyClient::call(request, [](auto const &response) {
            EXPECT_EQ("{\"r\":\"LOGGED-WITH-REMEMBER\"}", response);
        });
    }).join();
    // clang-format on
}

TEST_F(GeneralTest, ProxyClientTestLoginWithNullData)
{
    auto request = R"({"f":"sample.login","p":[{"n":"username","v":null},{"n":"password","v":null},{"n":"remember","v":null}]})";

    // clang-format off
    ProxyClient::call(request, [](auto const &response) {
        EXPECT_EQ("{\"r\":\"NOT-LOGGED\"}", response);
    });
    // clang-format on
}

TEST_F(GeneralTest, ProxyClientTestAsync)
{
    auto request = R"({"f":"sample.async","p":[]})";

    // clang-format off
    ProxyClient::call(request, [](auto const &response) {
        EXPECT_EQ("{\"r\":null}", response);
    });
    // clang-format on
}

TEST_F(GeneralTest, ProxyClientTestAsyncWithThread)
{
    auto request = R"({"f":"sample.async","p":[]})";

    // clang-format off
    std::thread([=] {
        ProxyClient::call(request, [](const auto &response) {
            EXPECT_EQ("{\"r\":null}", response);
        });
    }).join();
    // clang-format on
}

TEST_F(GeneralTest, ProxyClientTestNotFound)
{
    auto request = R"({"f":"not.found","p":[]})";

    // clang-format off
    ProxyClient::call(request, [](auto const &response) {
        EXPECT_EQ("", response);
    });
    // clang-format on
}
