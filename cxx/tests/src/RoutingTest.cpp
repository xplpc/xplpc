#include "fixtures/GeneralTest.hpp"
#include "fixtures/LogRecorder.hpp"
#include "fixtures/ScopedLayer.hpp"
#include "fixtures/SpyMapping.hpp"
#include "fixtures/VerifiedCall.hpp"
#include "xplpc/xplpc.hpp"
#include "gtest/gtest.h"

#include <optional>
#include <string>
#include <vector>

using namespace xplpc::client;
using namespace xplpc::core;
using namespace xplpc::map;
using namespace xplpc::message;

TEST_F(GeneralTest, RoutingRefusesARequestEveryBridgeWouldNotReadTheSameWay)
{
    // The router scans where every other bridge parses, so any document the two could read differently is refused before a mapping runs, and these are the shapes the refuse cases do not carry.

    SpyMapping::add();

    const std::vector<std::string> refused = {
        R"({"f":{"name":"test.spy"},"p":[]})",
        R"({"f":true,"p":[]})",
        R"({"f":null,"p":[]})",
        R"({"f":"test.spy","f":"test.spy","p":[]})",
        R"({"f":"other","f":"test.spy","p":[]})",
        R"({"f":["other"],"f":"test.spy","p":[]})",
        R"({"f":null,"f":"test.spy","p":[]})",
        R"({"f":"test.spy","p":[{"n":"a","n":"b","v":1}]})",
        R"({"f":"test.spy","p":{"n":"a","v":1}})",
        R"({"f":"test.spy","p":"a"})",
        R"({"f":"test.spy","p":[],"p":[]})",
        R"({"f":"test.spy","p":[{"n":"a","v":1,"v":2}]})",
        R"({"f":"test.spy","p":[{"n":"a","v":{"x":1,"x":2}}]})",
        R"({"f":"test.spy","p":[{"n":"a","v":[{"x":1,"x":2}]}]})",
        R"({"f":"test.spy","p":[],"x":1,"x":2})",
    };

    for (const auto &document : refused)
    {
        // clang-format off
        VerifiedCall::run(document, [&document](const std::string &response) {
            EXPECT_EQ("", response) << document;
        });
        // clang-format on
    }

    EXPECT_EQ(0, SpyMapping::count());
}

TEST_F(GeneralTest, RoutingIgnoresAFunctionNameThatIsNotAtTheRoot)
{
    SpyMapping::add();

    const std::vector<std::string> nested = {
        R"({"p":[{"n":"value","v":{"f":"test.spy"}}]})",
        R"({"x":{"f":"test.spy"}})",
        R"({"x":[{"f":"test.spy"}]})",
    };

    for (const auto &document : nested)
    {
        // clang-format off
        VerifiedCall::run(document, [&document](const std::string &response) {
            EXPECT_EQ("", response) << document;
        });
        // clang-format on
    }

    EXPECT_EQ(0, SpyMapping::count());
}

TEST_F(GeneralTest, RoutingSaysWhyARequestWasRefused)
{
    // A name that is not a string and a name that is missing end the same way for the caller, and only the log tells them apart.

    LogRecorder recorder;

    Client::call(R"({"f":["test.spy"],"p":[]})", [](const std::string &) {});

    const auto errors = recorder.errors();

    ASSERT_EQ(1u, errors.size());
    EXPECT_NE(std::string::npos, errors.front().find("the function name is not a string"));
}

TEST_F(GeneralTest, RoutingAnswersAnEmptyRequestWithoutReportingIt)
{
    // An empty request is what an encoder that failed answers with, and that encoder already reported it, so one event stays one line.

    LogRecorder recorder;

    const auto result = Router::shared().call(Router::shared().native(), "", nullptr);

    EXPECT_TRUE(result.isAnswered());
    EXPECT_EQ("", result.value());
    EXPECT_TRUE(recorder.reportedNothing()) << (recorder.errors().empty() ? "" : recorder.errors().front());
}

TEST_F(GeneralTest, RoutingReadsTheRootNameAfterParametersCarryingKeysOfTheirOwn)
{
    // A value nested in a parameter can carry an f or an n of its own, and the scan reads only the ones at the root.

    SpyMapping::add();

    // clang-format off
    VerifiedCall::run(R"({"p":[{"n":"a","v":1},{"n":"b","v":{"f":"x","n":"a"}}],"f":"test.spy"})", [](const std::string &response) {
        EXPECT_EQ(R"({"r":"ran"})", response);
    });
    // clang-format on

    EXPECT_EQ(1, SpyMapping::count());
}

TEST_F(GeneralTest, RoutingRefusesARequestBuiltWithAParameterNamedTwice)
{
    // Routing never scans a request built in c++, so the encoder is what refuses it.

    xplpc::message::Request request{
        "sample.echo",
        xplpc::message::Param<std::string>{"value", "first"},
        xplpc::message::Param<std::string>{"value", "second"},
    };

    EXPECT_EQ("", request.functionName());
    EXPECT_EQ("", request.data());

    // clang-format off
    VerifiedCall::run<std::string>(request, [](const std::optional<std::string> &response) {
        EXPECT_EQ(std::nullopt, response);
    });
    // clang-format on
}

TEST_F(GeneralTest, RoutingRefusesARequestBuiltWithNoFunctionNameOnce)
{
    // Every other bridge has the scan report a request naming no function, and a request built in c++ is never scanned, so the encoder is the one that says so.

    LogRecorder recorder;
    const xplpc::message::Request request{""};

    EXPECT_EQ("", request.data());

    // clang-format off
    VerifiedCall::run<std::string>(request, [](const std::optional<std::string> &response) {
        EXPECT_EQ(std::nullopt, response);
    });
    // clang-format on

    EXPECT_EQ(1u, recorder.errors().size());
}

TEST_F(GeneralTest, RoutingGivesANameToTheLayerThatDeclaredItLast)
{
    // Every host that registers a name later takes it over, and letting it go gives it back to the one before.

    // clang-format off
    MappingList::shared()->add("test.owner", Map::create<std::string>({}, [](const Message &, const Response &r) {
        r(std::string{"native"});
    }));

    const auto owner = []() {
        return Client::callSync(R"({"f":"test.owner","p":[]})");
    };
    // clang-format on

    ScopedLayer first(ScopedLayer::answering("first"));
    ScopedLayer second(ScopedLayer::answering("second"));

    EXPECT_EQ(R"({"r":"native"})", owner());

    first.declare("test.owner");
    EXPECT_EQ(R"({"r":"first"})", owner());

    second.declare("test.owner");
    EXPECT_EQ(R"({"r":"second"})", owner());

    second.retract("test.owner");
    EXPECT_EQ(R"({"r":"first"})", owner());

    second.declare("test.owner");
    EXPECT_EQ(R"({"r":"second"})", owner());

    // Declaring a name the layer already holds moves it to the top rather than leaving it where it was.
    first.declare("test.owner");
    EXPECT_EQ(R"({"r":"first"})", owner());

    second.retract("test.owner");
    EXPECT_EQ(R"({"r":"first"})", owner());

    first.detach();
    EXPECT_EQ(R"({"r":"native"})", owner());
}

TEST_F(GeneralTest, RoutingSendsANameAHostDeclaredToTheHostRatherThanToCxx)
{
    // The c++ mapping keeps its registration while the host holds the name, so it answers again once the host lets the name go.

    SpyMapping::add();

    ScopedLayer host(ScopedLayer::answering("host"));
    host.declare(SpyMapping::name);

    EXPECT_EQ(R"({"r":"host"})", Client::callSync(R"({"f":"test.spy","p":[]})"));
    EXPECT_EQ(0, SpyMapping::count());

    host.detach();

    EXPECT_EQ(R"({"r":"ran"})", Client::callSync(R"({"f":"test.spy","p":[]})"));
    EXPECT_EQ(1, SpyMapping::count());
}

TEST_F(GeneralTest, RoutingIgnoresARetractOfANameTheLayerDoesNotHold)
{
    SpyMapping::add();

    ScopedLayer bystander(ScopedLayer::answering("bystander"));
    bystander.retract(SpyMapping::name);

    EXPECT_EQ(R"({"r":"ran"})", Client::callSync(R"({"f":"test.spy","p":[]})"));
}

TEST_F(GeneralTest, RoutingRefusesADeclarationFromALayerThatIsNotAttached)
{
    LogRecorder recorder;

    SpyMapping::add();

    ScopedLayer gone(ScopedLayer::answering("gone"));
    gone.detach();
    gone.declare(SpyMapping::name);

    EXPECT_EQ(R"({"r":"ran"})", Client::callSync(R"({"f":"test.spy","p":[]})"));

    const auto errors = recorder.errors();

    ASSERT_EQ(1u, errors.size());
    EXPECT_NE(std::string::npos, errors.front().find("A layer that is not attached declared function"));
}
