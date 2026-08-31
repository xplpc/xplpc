#include "xplpc/c/platform.h"
#include "gtest/gtest.h"

class GeneralTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Every test starts from a registry it did not inherit, so a name one of them declares cannot route another one somewhere else.

        xplpc_core_initialize(true, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
        xplpc_native_clear_mappings();
    }
};
