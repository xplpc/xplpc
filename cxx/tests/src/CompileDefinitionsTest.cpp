#include "fixtures/GeneralTest.hpp"
#include "fixtures/VerifiedCall.hpp"
#include "xplpc/xplpc.hpp"
#include "gtest/gtest.h"

#include <string>

using namespace xplpc::core;
using namespace xplpc::client;
using namespace xplpc::message;

TEST_F(GeneralTest, CompileDefinitionsReachTheLibrary)
{
    // The version is defined on the library target, and the sample answers it from there.

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
