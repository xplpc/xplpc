#include "xplpc/custom/Mapping.hpp"
#include "xplpc/xplpc.hpp"
#include "gtest/gtest.h"

using namespace xplpc::core;
using namespace xplpc::custom;
using namespace xplpc::client;

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
        XPLPC::initialize();

        // initialize
        PlatformProxy::createDefault();
        PlatformProxy::shared()->initialize();
    }

    void TearDown() override
    {
        // code here will be called immediately after each test (right before the destructor)
    }

    // class members declared here can be used by all tests in the test suite
};
