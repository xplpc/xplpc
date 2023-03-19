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

        auto proxy = std::make_shared<NativePlatformProxy>();
        proxy->initialize();

        PlatformProxyList::shared()->append(proxy);
    }

    void TearDown() override
    {
        // code here will be called immediately after each test (right before the destructor)
    }

    // class members declared here can be used by all tests in the test suite
};
