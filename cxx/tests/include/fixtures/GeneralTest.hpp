#pragma once

#include "xplpc/custom/Mapping.hpp"
#include "xplpc/xplpc.hpp"
#include "gtest/gtest.h"

#include <memory>

using namespace xplpc::core;
using namespace xplpc::data;
using namespace xplpc::proxy;

class GeneralTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Every test starts from a clean registry, so leaked callbacks and stacked proxies show up instead of accumulating.

        PlatformProxyList::shared()->clear();
        MappingList::shared()->clear();
        CallbackList::shared()->clear();

        auto proxy = std::make_shared<NativePlatformProxy>();
        proxy->initialize();

        PlatformProxyList::shared()->append(proxy);

        XPLPC::initialize();
    }

    void TearDown() override
    {
        // A leaked callback is the defect this project has found most often, so every test is held to leaving none.

        EXPECT_EQ(CallbackList::shared()->count(), 0u);
    }
};
