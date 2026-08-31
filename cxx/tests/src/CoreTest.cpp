#include "fixtures/GeneralTest.hpp"
#include "xplpc/xplpc.hpp"
#include "gtest/gtest.h"

using namespace xplpc::core;
using namespace xplpc::data;
using namespace xplpc::proxy;

TEST(CoreTest, CoreIsNotReportedReadyUntilEverythingIsWired)
{
    // Bringing a proxy up registers what that side owns and says nothing about the library being able to serve a call, so the flag is the last step.

    XPLPC::finalize();
    ASSERT_FALSE(XPLPC::isInitialized());

    auto proxy = std::make_shared<NativePlatformProxy>();
    proxy->initialize();

    EXPECT_FALSE(XPLPC::isInitialized());

    PlatformProxyList::shared()->append(proxy);
    XPLPC::initialize();

    EXPECT_TRUE(XPLPC::isInitialized());
}

TEST(CoreTest, CorePublishesItsLoggerWhenItComesUp)
{
    // The guide tells a host to take this logger out of the registry right after bringing the library up, so it has to be there by then.

    XPLPC::initialize();

    ASSERT_NE(nullptr, spdlog::get("XPLPC"));
    EXPECT_EQ(spdlog::get("XPLPC"), xplpc::util::Log::logger());
}
