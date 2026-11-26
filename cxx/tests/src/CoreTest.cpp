#include "xplpc/xplpc.hpp"
#include "gtest/gtest.h"

#include "spdlog/spdlog.h"

using namespace xplpc::core;

TEST(CoreTest, RouterPublishesItsLoggerWhenItComesUp)
{
    // The guide tells a host to take this logger out of the registry before it uses the library, so it has to be there once the router exists.

    Router::shared();

    ASSERT_NE(nullptr, spdlog::get("XPLPC"));
    EXPECT_EQ(spdlog::get("XPLPC"), xplpc::util::Log::logger());
}

TEST(CoreTest, RouterAttachesTheNativeLayerBeforeAnyOther)
{
    const auto native = Router::shared().native();

    EXPECT_NE(0u, native);
    EXPECT_NE(nullptr, Router::shared().layer(native));
    EXPECT_EQ(nullptr, Router::shared().layer(0));
}
