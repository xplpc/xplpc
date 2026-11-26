#include "fixtures/GeneralTest.hpp"
#include "fixtures/Requests.hpp"
#include "fixtures/TestHost.hpp"
#include "xplpc/xplpc.hpp"
#include "gtest/gtest.h"

#include <memory>
#include <string>
#include <thread>

TEST_F(GeneralTest, RoutingGivesAContestedNameToTheHostRatherThanTheCore)
{
    // The c++ mappings are declared before any host attaches, so a host declaring the same name afterwards is the one that receives it.

    TestHost host(true, TestHost::Serving::Inline, R"({"r":"from-host"})");
    host.declare("sample.login");

    TestHost caller;

    const auto outcome = caller.call(1, Requests::login());

    EXPECT_TRUE(outcome.answered);
    EXPECT_EQ(R"({"r":"from-host"})", outcome.value);

    ASSERT_EQ(1u, host.invocations().size());
    EXPECT_EQ(Requests::login(), host.invocations()[0].request);
}

TEST_F(GeneralTest, RoutingHandsANameBetweenTwoHostsInDeclarationOrder)
{
    // Each name holds a stack of owners, so the last declaration wins and retracting or detaching gives the name back to the one below.

    TestHost caller;

    auto first = std::make_unique<TestHost>(true, TestHost::Serving::Inline, R"({"r":"from-first"})");
    auto second = std::make_unique<TestHost>(true, TestHost::Serving::Inline, R"({"r":"from-second"})");

    // clang-format off
    const auto answer = [&caller] {
        return caller.call(1, Requests::login()).value;
    };
    // clang-format on

    EXPECT_EQ(Requests::loginAnswer(), answer());

    first->declare("sample.login");
    EXPECT_EQ(R"({"r":"from-first"})", answer());

    second->declare("sample.login");
    EXPECT_EQ(R"({"r":"from-second"})", answer());

    first->declare("sample.login");
    EXPECT_EQ(R"({"r":"from-first"})", answer());

    first->retract("sample.login");
    EXPECT_EQ(R"({"r":"from-second"})", answer());

    second->retract("sample.login");
    EXPECT_EQ(Requests::loginAnswer(), answer());

    first->declare("sample.login");
    second->declare("sample.login");
    EXPECT_EQ(R"({"r":"from-second"})", answer());

    second->detach();
    EXPECT_TRUE(second->waitForRelease(5000));
    EXPECT_EQ(R"({"r":"from-first"})", answer());

    first->detach();
    EXPECT_TRUE(first->waitForRelease(5000));
    EXPECT_EQ(Requests::loginAnswer(), answer());
}

TEST_F(GeneralTest, RoutingRetractingANameTheHostDoesNotHoldChangesNothing)
{
    TestHost owner(true, TestHost::Serving::Inline, R"({"r":"from-owner"})");
    TestHost other(true, TestHost::Serving::Inline, R"({"r":"from-other"})");
    TestHost caller;

    owner.declare("host.retract");
    other.retract("host.retract");

    EXPECT_EQ(R"({"r":"from-owner"})", caller.call(1, Requests::named("host.retract")).value);
}

TEST_F(GeneralTest, RoutingReachesAHostFromAThreadItDidNotStartThroughPost)
{
    // A host that cannot be entered from a thread it did not start receives the call through post, with bytes it owns.

    TestHost host(false, TestHost::Serving::Inline, R"({"r":"from-host-off-thread"})");
    host.declare("host.off.thread");

    std::string answer;
    std::thread::id thread;

    // clang-format off
    std::thread([&answer, &thread] {
        thread = std::this_thread::get_id();

        xplpc::client::Client::call(Requests::named("host.off.thread"), [&answer](const std::string &response) {
            answer = response;
        });
    }).join();
    // clang-format on

    EXPECT_EQ(R"({"r":"from-host-off-thread"})", answer);
    EXPECT_EQ(1u, host.count(XPLPC_POST_INVOKE));

    const auto invocations = host.invocations();

    ASSERT_EQ(1u, invocations.size());
    EXPECT_TRUE(invocations[0].owned);
    EXPECT_EQ(thread, invocations[0].thread);
    EXPECT_EQ(Requests::named("host.off.thread"), invocations[0].request);
}

TEST_F(GeneralTest, RoutingEntersAHostThatCanBeEnteredAnywhereDirectly)
{
    // The same call made from the same thread reaches a host that said it can be entered anywhere through invoke, with borrowed bytes.

    TestHost host(true, TestHost::Serving::Inline, R"({"r":"from-host-anywhere"})");
    host.declare("host.anywhere");

    std::string answer;

    // clang-format off
    std::thread([&answer] {
        xplpc::client::Client::call(Requests::named("host.anywhere"), [&answer](const std::string &response) {
            answer = response;
        });
    }).join();
    // clang-format on

    EXPECT_EQ(R"({"r":"from-host-anywhere"})", answer);
    EXPECT_EQ(0u, host.count(XPLPC_POST_INVOKE));

    const auto invocations = host.invocations();

    ASSERT_EQ(1u, invocations.size());
    EXPECT_FALSE(invocations[0].owned);
}

TEST_F(GeneralTest, RoutingEntersOnlyTheHostWhoseCallIsRunning)
{
    // The thread scope records which layer started the call, so a thread running one host's call is not a thread another host can be entered on.

    TestHost server(false, TestHost::Serving::Inline, R"({"r":"from-server"})");
    server.declare("host.server");

    TestHost caller;

    const auto outcome = caller.call(1, Requests::named("host.server"));

    EXPECT_TRUE(outcome.answered);
    EXPECT_EQ(R"({"r":"from-server"})", outcome.value);

    const auto invocations = server.invocations();

    ASSERT_EQ(1u, invocations.size());
    EXPECT_TRUE(invocations[0].owned);
}

TEST_F(GeneralTest, RoutingEntersAHostOnTheThreadRunningItsOwnCall)
{
    // A host calling a name it declared is running its own call on this thread, so it is entered directly even though it cannot be entered anywhere.

    TestHost host(false, TestHost::Serving::Inline, R"({"r":"from-self"})");
    host.declare("host.self");

    const auto outcome = host.call(1, Requests::named("host.self"));

    EXPECT_TRUE(outcome.answered);
    EXPECT_EQ(R"({"r":"from-self"})", outcome.value);

    const auto invocations = host.invocations();

    ASSERT_EQ(1u, invocations.size());
    EXPECT_FALSE(invocations[0].owned);
}
