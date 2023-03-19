#include "xplpc/c/platform.h"
#include "gtest/gtest.h"

class GeneralTest : public ::testing::Test
{
protected:
    GeneralTest()
    {
        // you can do set-up work for each test here.
    }

    ~GeneralTest() override
    {
        // cleanup any pending stuff, but no exceptions allowed
    }

    void SetUp() override
    {
        // code here will be called immediately after the constructor (right before each test)
        xplpc_core_initialize(true, nullptr, nullptr, nullptr, nullptr, nullptr);
    }

    void TearDown() override
    {
        // code here will be called immediately after each test (right before the destructor)
    }

    // class members declared here can be used by all tests in the test suite
};
