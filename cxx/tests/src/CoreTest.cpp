#include "fixtures/GeneralTest.hpp"
#include "xplpc/xplpc.hpp"
#include "gtest/gtest.h"

using namespace xplpc::core;

TEST(CoreTest, CoreTestInitialize)
{
    auto proxy = std::make_shared<NativePlatformProxy>();
    proxy->initialize();

    EXPECT_EQ(true, XPLPC::isInitialized());
}
