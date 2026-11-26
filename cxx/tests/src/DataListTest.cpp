#include "fixtures/GeneralTest.hpp"
#include "xplpc/xplpc.hpp"
#include "gtest/gtest.h"

#include <string>

using namespace xplpc::client;
using namespace xplpc::core;
using namespace xplpc::data;
using namespace xplpc::map;
using namespace xplpc::message;

TEST_F(GeneralTest, MappingListSurvivesClear)
{
    EXPECT_GT(MappingList::shared()->count(), 0);

    MappingList::shared()->clear();

    EXPECT_NE(nullptr, MappingList::shared());
    EXPECT_EQ(0, MappingList::shared()->count());
    EXPECT_FALSE(MappingList::shared()->has("sample.login"));

    // Clearing retracts every name it held, so the router has nobody left to route it to.
    EXPECT_EQ("", Client::callSync(R"({"f":"sample.login","p":[]})"));

    // The mappings are restored so the remaining tests keep working.
    xplpc::custom::Mapping::initialize();

    EXPECT_TRUE(MappingList::shared()->has("sample.login"));
}

TEST_F(GeneralTest, ClientAcceptsEmptyCallbackForUnknownFunction)
{
    auto request = Request{"sample.function.that.does.not.exist"};
    Client::call<std::string>(request, nullptr);

    EXPECT_EQ(0u, Router::shared().pending());
}

TEST_F(GeneralTest, DataViewNamingNoBufferReadsNothing)
{
    // A view decoded from a document that carried no handle names nothing, and reading it answers nothing rather than an address.

    EXPECT_FALSE(xplpc::type::DataView{}.bytes().has_value());
}
