#include "fixtures/GeneralTest.hpp"
#include "fixtures/VerifiedCall.hpp"
#include "xplpc/xplpc.hpp"
#include "gtest/gtest.h"

#include <string>

using namespace xplpc::core;
using namespace xplpc::client;
using namespace xplpc::message;

std::string getVersion()
{
    return std::string(XPLPC_VERSION) + " (" + std::string(XPLPC_VERSION_CODE) + ")";
}

std::string getTarget()
{
    return std::string(XPLPC_TARGET);
}

TEST_F(GeneralTest, CompileDefinitionsTestVersion)
{
    auto version = getVersion();

    EXPECT_EQ("1.0.0 (1)", version);
}

TEST_F(GeneralTest, CompileDefinitionsTestTarget)
{
    auto target = getTarget();

    EXPECT_NE("", target);
}

TEST_F(GeneralTest, TestVersion)
{
    auto request = Request{"sample.version"};

    // clang-format off
    VerifiedCall::run<std::string>(request, [](const auto &response) {
        EXPECT_NE(response, std::nullopt);

        if (response)
        {
            auto version = response.value();
            EXPECT_EQ("1.0.0 (1)", version);
        }
    });
    // clang-format on
}

TEST_F(GeneralTest, TestTarget)
{
    auto request = Request{"sample.target"};

    // clang-format off
    VerifiedCall::run<std::string>(request, [](const auto &response) {
        EXPECT_NE(response, std::nullopt);

        if (response)
        {
            auto target = response.value();
            EXPECT_NE("", target);
        }
    });
    // clang-format on
}
