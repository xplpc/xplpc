#pragma once

#include "xplpc/custom/Mapping.hpp"
#include "xplpc/xplpc.hpp"
#include "gtest/gtest.h"

using namespace xplpc::core;
using namespace xplpc::data;

class GeneralTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Every test starts from the sample mappings alone, so a name a previous test declared cannot answer for this one.
        // The router registers the c++ mappings on the first call of the process, so a call carrying no name makes that happen here rather than in the middle of whichever test calls first.

        Router::shared().call(Router::shared().native(), "", "", nullptr);
        MappingList::shared()->clear();
        xplpc::custom::Mapping::initialize();
    }

    void TearDown() override
    {
        // A call left waiting is the defect this project has found most often, so every test is held to leaving none.

        EXPECT_EQ(Router::shared().pending(), 0u);
    }
};
