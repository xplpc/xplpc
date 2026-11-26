#include "fixtures/GeneralTest.hpp"
#include "fixtures/HeldResponders.hpp"
#include "fixtures/Requests.hpp"
#include "fixtures/TestHost.hpp"
#include "xplpc/xplpc.hpp"
#include "gtest/gtest.h"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <memory>
#include <string>
#include <thread>
#include <vector>

using namespace xplpc::data;
using namespace xplpc::map;
using namespace xplpc::message;

TEST_F(GeneralTest, ClientTestLogin)
{
    TestHost host;

    const auto outcome = host.call(1, Requests::login());

    EXPECT_TRUE(outcome.answered);
    EXPECT_EQ(Requests::loginAnswer(), outcome.value);
    EXPECT_EQ(0u, host.count(XPLPC_POST_ANSWER));
}

TEST_F(GeneralTest, ClientTestLoginFromAnotherThread)
{
    TestHost host;
    TestHost::Outcome outcome{};

    // clang-format off
    std::thread([&host, &outcome] {
        outcome = host.call(1, Requests::login());
    }).join();
    // clang-format on

    EXPECT_TRUE(outcome.answered);
    EXPECT_EQ(Requests::loginAnswer(), outcome.value);
}

TEST_F(GeneralTest, ClientCarriesEveryUtf8WidthThroughTheAbi)
{
    // The abi carries a pointer and a size and never a terminator, so a payload wider than ascii has to survive byte for byte.

    TestHost host;

    const std::string value = "acentuacao ç 日本語 \U0001f600";
    const std::string request = R"({"f":"sample.echo","p":[{"n":"value","v":")" + value + R"("}]})";

    const auto outcome = host.call(1, request);

    EXPECT_TRUE(outcome.answered);
    EXPECT_EQ(R"({"r":")" + value + R"("})", outcome.value);
}

TEST_F(GeneralTest, ClientRespectsTheBufferSize)
{
    // The buffers are deliberately not null terminated, so any use of strlen would read past the end.

    TestHost host;

    const auto &request = Requests::login();
    std::vector<char> buffer(request.begin(), request.end());

    const auto outcome = host.call(1, buffer.data(), buffer.size());

    EXPECT_TRUE(outcome.answered);
    EXPECT_EQ(Requests::loginAnswer(), outcome.value);
}

TEST_F(GeneralTest, ClientAnswersEmptyForAFunctionNothingOwns)
{
    // Nothing owns this name, so the caller is answered with the empty value rather than left waiting.

    TestHost host;

    const auto outcome = host.call(1, Requests::named("not.found"));

    EXPECT_TRUE(outcome.answered);
    EXPECT_EQ("", outcome.value);
}

TEST_F(GeneralTest, ClientAnswersWhenAMappingRaises)
{
    // A mapping is free to raise something that does not derive from std::exception, and the caller still has to be answered.

    // clang-format off
    MappingList::shared()->add("test.raises", Map::create<std::string>({}, [](const Message &, const Response &) {
        throw 42;
    }));
    // clang-format on

    TestHost host;

    const auto outcome = host.call(1, Requests::named("test.raises"));

    EXPECT_TRUE(outcome.answered);
    EXPECT_EQ("", outcome.value);
}

TEST_F(GeneralTest, ClientDoesNotBlockOnAMappingThatAnswersLater)
{
    // A mapping that answers after the call has returned is the genuinely asynchronous case, and the answer arrives through post exactly once.

    auto held = std::make_shared<HeldResponders>();

    // clang-format off
    MappingList::shared()->add("test.held", Map::create<std::string>({}, [held](const Message &, const Response &r) {
        held->add(r);
    }));
    // clang-format on

    TestHost host;

    const auto outcome = host.call(7, Requests::named("test.held"));

    EXPECT_FALSE(outcome.answered);
    EXPECT_NE(0u, outcome.call);
    EXPECT_EQ(0u, host.count(XPLPC_POST_ANSWER));

    held->answerAll("detached");

    const auto answers = host.answers();

    ASSERT_EQ(1u, answers.size());
    EXPECT_EQ(7u, answers[0].id);
    EXPECT_EQ(R"({"r":"detached"})", answers[0].data);
}

TEST_F(GeneralTest, ClientCarriesANullAnswerFromAnotherThread)
{
    // The mapping answers null from a thread it created, which the host only ever receives through post.

    TestHost host;

    const auto outcome = host.call(9, Requests::named("sample.async"));

    EXPECT_FALSE(outcome.answered);

    ASSERT_TRUE(host.waitForPosts(XPLPC_POST_ANSWER, 1, 5000));

    const auto answers = host.answers();

    ASSERT_EQ(1u, answers.size());
    EXPECT_EQ(9u, answers[0].id);
    EXPECT_EQ(R"({"r":null})", answers[0].data);
}

TEST_F(GeneralTest, ClientCarriesTheTokenBackUntouched)
{
    // The token is the host's to choose, so every value it can hold has to come back as it went.

    TestHost server(true, TestHost::Serving::Later, R"({"r":"later"})");
    server.declare("host.later");

    TestHost caller;

    const std::vector<std::uint64_t> tokens = {0, 1, std::numeric_limits<std::uint64_t>::max()};

    for (const auto token : tokens)
    {
        EXPECT_FALSE(caller.call(token, Requests::named("host.later")).answered);
    }

    server.releaseLater();

    ASSERT_TRUE(caller.waitForPosts(XPLPC_POST_ANSWER, tokens.size(), 5000));
    server.joinWorkers();

    std::vector<std::uint64_t> answered;

    for (const auto &answer : caller.answers())
    {
        EXPECT_EQ(R"({"r":"later"})", answer.data);
        answered.push_back(answer.id);
    }

    std::sort(answered.begin(), answered.end());
    EXPECT_EQ(tokens, answered);
}

TEST_F(GeneralTest, ClientReceivesAnInlineAnswerAsTheValueOfTheCall)
{
    // A host answering before its invoke returns is answered through the value of the call, and post never carries it.

    TestHost server(true, TestHost::Serving::Inline, R"({"r":"inline"})");
    server.declare("host.inline");

    TestHost caller;

    const auto outcome = caller.call(1, Requests::named("host.inline"));

    EXPECT_TRUE(outcome.answered);
    EXPECT_EQ(R"({"r":"inline"})", outcome.value);
    EXPECT_EQ(0u, caller.count(XPLPC_POST_ANSWER));
}

TEST_F(GeneralTest, ClientReceivesADeferredAnswerFromAHostThreadExactlyOnce)
{
    // The host takes the call and answers from a thread of its own, which is what a network request looks like here.

    TestHost server(true, TestHost::Serving::Later, R"({"r":"from-host-later"})");
    server.declare("host.deferred");

    TestHost caller;

    const auto outcome = caller.call(42, Requests::named("host.deferred"));

    EXPECT_FALSE(outcome.answered);
    EXPECT_EQ(0u, caller.count(XPLPC_POST_ANSWER));

    server.releaseLater();

    ASSERT_TRUE(caller.waitForPosts(XPLPC_POST_ANSWER, 1, 5000));
    server.joinWorkers();

    const auto answers = caller.answers();

    ASSERT_EQ(1u, answers.size());
    EXPECT_EQ(42u, answers[0].id);
    EXPECT_EQ(R"({"r":"from-host-later"})", answers[0].data);
}

TEST_F(GeneralTest, ClientCancelMakesALaterAnswerANoOp)
{
    // A cancelled call is gone from the router, so the answer the server gives afterwards resolves nothing and post never carries it.

    TestHost server(true, TestHost::Serving::Never);
    server.declare("host.never");

    TestHost caller;

    const auto outcome = caller.call(5, Requests::named("host.never"));

    ASSERT_FALSE(outcome.answered);
    ASSERT_EQ(1u, server.invocations().size());
    EXPECT_EQ(outcome.call, server.invocations()[0].call);

    EXPECT_TRUE(xplpc_cancel(caller.handle(), outcome.call));
    EXPECT_FALSE(xplpc_cancel(caller.handle(), outcome.call));

    const std::string late = R"({"r":"too-late"})";
    xplpc_answer(server.invocations()[0].call, late.data(), late.size());

    EXPECT_EQ(0u, caller.count(XPLPC_POST_ANSWER));
}

TEST_F(GeneralTest, ClientCancelOfADeferredCoreMappingNeverPosts)
{
    // The c++ mapping answers from its own thread after the call was cancelled, and that answer reaches nobody.

    auto held = std::make_shared<HeldResponders>();

    // clang-format off
    MappingList::shared()->add("test.held", Map::create<std::string>({}, [held](const Message &, const Response &r) {
        held->add(r);
    }));
    // clang-format on

    TestHost host;

    const auto outcome = host.call(3, Requests::named("test.held"));

    ASSERT_FALSE(outcome.answered);
    EXPECT_TRUE(xplpc_cancel(host.handle(), outcome.call));

    held->answerAll("detached");

    EXPECT_EQ(0u, host.count(XPLPC_POST_ANSWER));
}

TEST_F(GeneralTest, ClientRefusesACancelFromTheHostServingTheCall)
{
    // The host serving a call holds its handle too, and taking the call back from there would leave the caller waiting for good.

    TestHost server(true, TestHost::Serving::Never);
    server.declare("host.never");

    TestHost caller;

    const auto outcome = caller.call(5, Requests::named("host.never"));

    ASSERT_FALSE(outcome.answered);
    ASSERT_EQ(1u, server.invocations().size());

    EXPECT_FALSE(xplpc_cancel(server.handle(), outcome.call));
    EXPECT_FALSE(xplpc_cancel(xplpc::core::Router::shared().native(), outcome.call));
    EXPECT_EQ(1u, xplpc_pending());

    const std::string value = R"({"r":"answered"})";
    xplpc_answer(outcome.call, value.data(), value.size());

    ASSERT_EQ(1u, caller.answers().size());
    EXPECT_EQ(value, caller.answers()[0].data);
}

TEST_F(GeneralTest, ClientCancelAfterTheAnswerTakesNothing)
{
    TestHost server(true, TestHost::Serving::Never);
    server.declare("host.never");

    TestHost caller;

    const auto outcome = caller.call(5, Requests::named("host.never"));

    ASSERT_FALSE(outcome.answered);

    const std::string value = R"({"r":"answered"})";
    xplpc_answer(outcome.call, value.data(), value.size());

    EXPECT_FALSE(xplpc_cancel(caller.handle(), outcome.call));
    ASSERT_EQ(1u, caller.answers().size());
    EXPECT_EQ(value, caller.answers()[0].data);
}

TEST_F(GeneralTest, NestedCoreMappingReachesTheHostAndAnswersTheOuterCallInline)
{
    // A core mapping needs a value only the host has, so the outer answer is built from an inner call that crossed the abi on the same thread.

    TestHost host(false, TestHost::Serving::Inline, R"({"r":"from-host"})");
    host.declare("platform.reverse.response");

    const auto outcome = host.call(1, Requests::named("sample.reverse"));

    EXPECT_TRUE(outcome.answered);
    EXPECT_EQ(R"({"r":"response-is-from-host"})", outcome.value);

    ASSERT_EQ(1u, host.invocations().size());
    EXPECT_FALSE(host.invocations()[0].owned);
    EXPECT_EQ(0u, host.count(XPLPC_POST_ANSWER));
}

TEST_F(GeneralTest, NestedCoreMappingReachesAHostThatAnswersLater)
{
    // The host takes time over the inner call, and the outer answer still reaches the caller exactly once when it arrives.

    TestHost host(false, TestHost::Serving::Later, R"({"r":"from-host-later"})");
    host.declare("platform.reverse.response");

    const auto outcome = host.call(11, Requests::named("sample.reverse"));

    EXPECT_FALSE(outcome.answered);

    host.releaseLater();

    ASSERT_TRUE(host.waitForPosts(XPLPC_POST_ANSWER, 1, 5000));
    host.joinWorkers();

    const auto answers = host.answers();

    ASSERT_EQ(1u, answers.size());
    EXPECT_EQ(11u, answers[0].id);
    EXPECT_EQ(R"({"r":"response-is-from-host-later"})", answers[0].data);

    ASSERT_EQ(1u, host.invocations().size());
    EXPECT_FALSE(host.invocations()[0].owned);
}

TEST_F(GeneralTest, ClientCountsTheCallsStillWaiting)
{
    // The count is what lets a host prove it left nothing behind, so it moves with a call that waits and comes back when the call is taken back.

    TestHost server(true, TestHost::Serving::Never);
    server.declare("host.never");

    TestHost caller;

    EXPECT_EQ(0u, xplpc_pending());

    const auto outcome = caller.call(9, Requests::named("host.never"));

    ASSERT_FALSE(outcome.answered);
    EXPECT_EQ(1u, xplpc_pending());

    EXPECT_TRUE(xplpc_cancel(caller.handle(), outcome.call));
    EXPECT_EQ(0u, xplpc_pending());
}
