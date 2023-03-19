#include "fixtures/GeneralTest.hpp"
#include "xplpc/c/platform.h"
#include "gtest/gtest.h"

TEST(CoreTest, CoreTestInitialize)
{
    xplpc_core_initialize(true, nullptr, nullptr, nullptr, nullptr, nullptr);
    EXPECT_EQ(true, xplpc_core_is_initialized());
}

TEST(CoreTest, CoreTestInitializeAndFinalize)
{
    xplpc_core_initialize(true, nullptr, nullptr, nullptr, nullptr, nullptr);
    EXPECT_EQ(true, xplpc_core_is_initialized());
    xplpc_core_finalize();
}
