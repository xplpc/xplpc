#pragma once

#include "xplpc/core/Router.hpp"
#include "gtest/gtest.h"

class GeneralTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        ASSERT_EQ(0u, xplpc::core::Router::shared().pending());
    }

    // Every test detaches what it attached and settles every call it made, so nothing is left for the next one to inherit.
    void TearDown() override
    {
        EXPECT_EQ(0u, xplpc::core::Router::shared().pending());
    }
};
