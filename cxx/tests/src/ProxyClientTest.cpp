#include "fixtures/GeneralTest.hpp"
#include "xplpc/xplpc.hpp"
#include "gtest/gtest.h"

using namespace xplpc::core;
using namespace xplpc::client;

TEST_F(GeneralTest, ProxyClientTestLogin)
{
    auto request = R"({"f":"sample.login","p":[{"n":"username","v":"paulo"},{"n":"password","v":"123456"},{"n":"remember","v":true}]})";
    auto ret = ProxyClient::call(request);

    EXPECT_EQ("{\"r\":\"LOGGED-WITH-REMEMBER\"}", ret);
}

TEST_F(GeneralTest, ProxyClientTestLoginAsync)
{
    auto request = R"({"f":"sample.login","p":[{"n":"username","v":"paulo"},{"n":"password","v":"123456"},{"n":"remember","v":true}]})";
    auto ret = ProxyClient::callAsync(request).get();

    EXPECT_EQ("{\"r\":\"LOGGED-WITH-REMEMBER\"}", ret);
}
