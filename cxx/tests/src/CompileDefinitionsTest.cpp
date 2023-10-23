#include "fixtures/GeneralTest.hpp"
#include "xplpc/xplpc.hpp"
#include "gtest/gtest.h"

#include <string>

using namespace xplpc::core;
using namespace xplpc::client;
using namespace xplpc::message;

std::string getVersion()
{
#ifdef XPLPC_VERSION
#define XPLPC_VERSION_STR XPLPC_VERSION
#else
#define XPLPC_VERSION_STR "0.0.0"
#endif

#ifdef XPLPC_VERSION_CODE
#define XPLPC_VERSION_CODE_STR XPLPC_VERSION_CODE
#else
#define XPLPC_VERSION_CODE_STR "0"
#endif

    return std::string(XPLPC_VERSION_STR) + " (" + std::string(XPLPC_VERSION_CODE_STR) + ")";
}

std::string getTarget()
{
#ifdef XPLPC_TARGET
#define XPLPC_TARGET_STR XPLPC_TARGET
#else
#define XPLPC_TARGET_STR ""
#endif

    return std::string(XPLPC_TARGET_STR);
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
    Client::call<std::string>(request, [](const auto &response) {
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
    Client::call<std::string>(request, [](const auto &response) {
        EXPECT_NE(response, std::nullopt);

        if (response)
        {
            auto target = response.value();
            EXPECT_NE("", target);
            EXPECT_NE("unknown", target);
        }
    });
    // clang-format on
}
