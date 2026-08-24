#include "fixtures/GeneralTest.hpp"
#include "xplpc/c/platform.h"
#include "gtest/gtest.h"

TEST(CoreTest, CoreTestInitialize)
{
    xplpc_core_initialize(true, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
    EXPECT_EQ(true, xplpc_core_is_initialized());
}

TEST(CoreTest, CoreTestInitializeAndFinalize)
{
    // The flag says whether the library can serve a call right now, so finalizing has to take it back and initializing again has to give it back.

    xplpc_core_initialize(true, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
    EXPECT_EQ(true, xplpc_core_is_initialized());

    xplpc_core_finalize();
    EXPECT_EQ(false, xplpc_core_is_initialized());

    xplpc_core_initialize(true, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
    EXPECT_EQ(true, xplpc_core_is_initialized());
}
