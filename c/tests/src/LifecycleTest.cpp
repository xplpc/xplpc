#include "fixtures/GeneralTest.hpp"
#include "fixtures/HeldResponders.hpp"
#include "fixtures/Requests.hpp"
#include "fixtures/TestHost.hpp"
#include "xplpc/layer/CLayer.hpp"
#include "xplpc/xplpc.hpp"
#include "gtest/gtest.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include <thread>
#include <vector>

using namespace xplpc::data;
using namespace xplpc::map;
using namespace xplpc::message;

TEST_F(GeneralTest, LifecycleDetachAnswersTheCallsAHostMadeBeforeItsRelease)
{
    // A mapping that never answers leaves a call pending, and after detach nothing can ever resolve it, so the host is answered while it can still be reached.

    // clang-format off
    MappingList::shared()->add("test.never.answers", Map::create<std::string>({}, [](const Message &, const Response &) {
    }));
    // clang-format on

    TestHost host;

    const auto outcome = host.call(21, Requests::named("test.never.answers"));

    ASSERT_FALSE(outcome.answered);
    EXPECT_EQ(1u, xplpc::core::Router::shared().pending());

    host.detach();

    ASSERT_TRUE(host.waitForRelease(5000));

    const auto posts = host.posts();

    ASSERT_EQ(2u, posts.size());
    EXPECT_EQ(XPLPC_POST_ANSWER, posts[0].kind);
    EXPECT_EQ(21u, posts[0].id);
    EXPECT_EQ("", posts[0].data);
    EXPECT_EQ(XPLPC_POST_RELEASE, posts[1].kind);
    EXPECT_EQ(0u, posts[1].id);
}

TEST_F(GeneralTest, LifecycleDetachAnswersTheCallsAHostWasServing)
{
    // The caller is waiting on a server that is going away, so it is answered with the empty value and the server's own answer afterwards reaches nobody.

    TestHost caller;
    TestHost server(true, TestHost::Serving::Never);
    server.declare("host.never");

    const auto outcome = caller.call(8, Requests::named("host.never"));

    ASSERT_FALSE(outcome.answered);

    server.detach();

    ASSERT_TRUE(server.waitForRelease(5000));

    const auto answers = caller.answers();

    ASSERT_EQ(1u, answers.size());
    EXPECT_EQ(8u, answers[0].id);
    EXPECT_EQ("", answers[0].data);

    const std::string late = R"({"r":"too-late"})";
    xplpc_answer(outcome.call, late.data(), late.size());

    EXPECT_EQ(1u, caller.answers().size());
    EXPECT_EQ(1u, server.posts().size());
}

TEST_F(GeneralTest, LifecycleDetachAnswersBeforeADeferredMappingDoes)
{
    // The c++ mapping is still working when the host detaches, so the host is answered with the empty value once and the mapping's answer is dropped.

    auto held = std::make_shared<HeldResponders>();

    // clang-format off
    MappingList::shared()->add("test.held", Map::create<std::string>({}, [held](const Message &, const Response &r) {
        held->add(r);
    }));
    // clang-format on

    TestHost host;

    ASSERT_FALSE(host.call(4, Requests::named("test.held")).answered);

    host.detach();

    ASSERT_TRUE(host.waitForRelease(5000));
    held->answerAll("detached");

    const auto posts = host.posts();

    ASSERT_EQ(2u, posts.size());
    EXPECT_EQ(XPLPC_POST_ANSWER, posts[0].kind);
    EXPECT_EQ("", posts[0].data);
    EXPECT_EQ(XPLPC_POST_RELEASE, posts.back().kind);
}

TEST_F(GeneralTest, LifecycleALayerThatWasNeverAttachedPostsNoRelease)
{
    // A host whose attach answered zero was refused and still owns its context, so a release reaching it would have it free that context twice.

    std::atomic<int> releases{0};

    // clang-format off
    const xplpc_host host{&releases, [](void *, xplpc_handle call, const char *, size_t) {
        xplpc_answer(call, "", 0);
    }, [](void *context, xplpc_post_kind kind, uint64_t, char *data, size_t) {
        if (kind == XPLPC_POST_RELEASE)
        {
            static_cast<std::atomic<int> *>(context)->fetch_add(1);
        }

        xplpc_free(data);
    }, false, nullptr, 0};
    // clang-format on

    std::make_shared<xplpc::layer::CLayer>(host).reset();

    EXPECT_EQ(0, releases.load());
}

TEST_F(GeneralTest, LifecycleReleaseIsTheLastPostAndArrivesOnce)
{
    // Nothing reaches a host once its layer is gone, so neither a late answer nor a call through the old handle posts anything after the release.

    TestHost server(true, TestHost::Serving::Never);
    server.declare("host.never");

    TestHost caller;

    for (std::uint64_t token = 1; token <= 3; token++)
    {
        ASSERT_FALSE(caller.call(token, Requests::named("host.never")).answered);
    }

    caller.detach();

    ASSERT_TRUE(caller.waitForRelease(5000));

    for (const auto &invocation : server.invocations())
    {
        const std::string late = R"({"r":"late"})";
        xplpc_answer(invocation.call, late.data(), late.size());
    }

    EXPECT_TRUE(caller.call(4, Requests::login()).answered);

    const auto posts = caller.posts();

    ASSERT_EQ(4u, posts.size());
    EXPECT_EQ(3u, caller.count(XPLPC_POST_ANSWER));
    EXPECT_EQ(1u, caller.count(XPLPC_POST_RELEASE));
    EXPECT_EQ(XPLPC_POST_RELEASE, posts.back().kind);
}

TEST_F(GeneralTest, LifecycleDetachFromInsideTheHostsOwnInvoke)
{
    // The dispatch holds the layer while the host is inside it, so detaching there needs no special case and the release arrives once the invoke returned.

    TestHost server(true, TestHost::Serving::DetachSelf);
    server.declare("host.detach.self");

    TestHost caller;

    const auto outcome = caller.call(1, Requests::named("host.detach.self"));

    EXPECT_TRUE(outcome.answered);
    EXPECT_EQ("", outcome.value);

    ASSERT_TRUE(server.waitForRelease(5000));

    const auto posts = server.posts();

    ASSERT_EQ(1u, posts.size());
    EXPECT_EQ(XPLPC_POST_RELEASE, posts[0].kind);
    EXPECT_EQ(1u, server.invocations().size());

    EXPECT_EQ(Requests::loginAnswer(), caller.call(2, Requests::login()).value);
}

TEST_F(GeneralTest, LifecycleDetachFromInsideThePostThatCarriedTheCall)
{
    // A host reached through post detaches from inside that post, and the release still arrives after it, exactly once.

    TestHost server(false, TestHost::Serving::DetachSelf);
    server.declare("host.detach.self");

    TestHost caller;

    const auto outcome = caller.call(1, Requests::named("host.detach.self"));

    EXPECT_TRUE(outcome.answered);
    EXPECT_EQ("", outcome.value);

    ASSERT_TRUE(server.waitForRelease(5000));

    const auto posts = server.posts();

    ASSERT_EQ(2u, posts.size());
    EXPECT_EQ(XPLPC_POST_INVOKE, posts[0].kind);
    EXPECT_EQ(XPLPC_POST_RELEASE, posts[1].kind);
}

TEST_F(GeneralTest, LifecycleCallThroughADetachedLayerRunsNoMapping)
{
    // A layer that detached can no longer start a call, so a mapping it names is never run on its behalf.

    static std::atomic<bool> executed = false;
    executed = false;

    // clang-format off
    MappingList::shared()->add("test.after.detach", Map::create<std::string>({}, [](const Message &, const Response &r) {
        executed = true;
        r(std::string("ran"));
    }));
    // clang-format on

    TestHost host;

    host.detach();
    ASSERT_TRUE(host.waitForRelease(5000));

    const auto outcome = host.call(1, Requests::named("test.after.detach"));

    EXPECT_TRUE(outcome.answered);
    EXPECT_EQ("", outcome.value);
    EXPECT_FALSE(executed);

    host.declare("test.after.detach");
    host.retract("test.after.detach");
    xplpc_detach(host.handle());

    EXPECT_EQ(1u, host.count(XPLPC_POST_RELEASE));
}

TEST_F(GeneralTest, LifecycleEachAttachIsALayerOfItsOwn)
{
    TestHost first;
    TestHost second;

    EXPECT_NE(0u, first.handle());
    EXPECT_NE(0u, second.handle());
    EXPECT_NE(first.handle(), second.handle());

    first.detach();
    ASSERT_TRUE(first.waitForRelease(5000));

    EXPECT_EQ(Requests::loginAnswer(), second.call(1, Requests::login()).value);
    EXPECT_EQ(0u, second.count(XPLPC_POST_RELEASE));
}

TEST_F(GeneralTest, LifecycleAHostAttachingUnderATakenNameReplacesItWithoutReachingIt)
{
    // A runtime restarted in the same process attaches under the name it had, and the host it replaces is gone, so nothing is posted to it, not even a release.

    // clang-format off
    MappingList::shared()->add("test.never.answers", Map::create<std::string>({}, [](const Message &, const Response &) {
    }));
    // clang-format on

    TestHost caller;
    TestHost previous(true, TestHost::Serving::Never, R"({"r":"from-previous"})", "isolate");
    previous.declare("host.replaced");

    const auto served = caller.call(31, Requests::named("host.replaced"));
    ASSERT_FALSE(served.answered);
    ASSERT_FALSE(previous.call(32, Requests::named("test.never.answers")).answered);
    ASSERT_EQ(2u, xplpc::core::Router::shared().pending());

    previous.expectReplaced();

    TestHost current(true, TestHost::Serving::Inline, R"({"r":"from-current"})", "isolate");

    EXPECT_NE(previous.handle(), current.handle());

    const auto answers = caller.answers();

    ASSERT_EQ(1u, answers.size());
    EXPECT_EQ(31u, answers[0].id);
    EXPECT_EQ("", answers[0].data);

    const auto routed = caller.call(33, Requests::named("host.replaced"));

    EXPECT_TRUE(routed.answered);
    EXPECT_EQ("", routed.value);

    const std::string late = R"({"r":"too-late"})";
    xplpc_answer(served.call, late.data(), late.size());

    current.declare("host.replaced");
    EXPECT_EQ(R"({"r":"from-current"})", caller.call(34, Requests::named("host.replaced")).value);
    EXPECT_EQ(1u, caller.answers().size());
}

TEST_F(GeneralTest, LifecycleHostsUnderDifferentNamesDoNotReplaceEachOther)
{
    TestHost first(true, TestHost::Serving::Inline, R"({"r":"from-first"})", "first");
    first.declare("host.first");

    TestHost second(true, TestHost::Serving::Inline, R"({"r":"from-second"})", "second");
    TestHost unnamed;

    EXPECT_EQ(R"({"r":"from-first"})", unnamed.call(1, Requests::named("host.first")).value);
    EXPECT_EQ(0u, first.count(XPLPC_POST_RELEASE));
}

TEST_F(GeneralTest, LifecycleAnAbandonedHostIsNeverReachedAgain)
{
    // A runtime that shuts down while it still owns names and still waits on calls is gone, so its layer leaves without anything being posted to it, not even a release.

    auto held = std::make_shared<HeldResponders>();

    // clang-format off
    MappingList::shared()->add("test.held", Map::create<std::string>({}, [held](const Message &, const Response &r) {
        held->add(r);
    }));
    // clang-format on

    TestHost caller;
    TestHost below(true, TestHost::Serving::Inline, R"({"r":"from-below"})");
    below.declare("host.abandoned");

    TestHost abandoned(true, TestHost::Serving::Never);
    abandoned.declare("host.abandoned");

    const auto served = caller.call(41, Requests::named("host.abandoned"));
    ASSERT_FALSE(served.answered);
    ASSERT_FALSE(abandoned.call(42, Requests::named("test.held")).answered);
    ASSERT_EQ(2u, xplpc::core::Router::shared().pending());

    abandoned.expectReplaced();

    const auto handle = abandoned.handle();
    xplpc_abandon(&handle);

    const auto answers = caller.answers();

    ASSERT_EQ(1u, answers.size());
    EXPECT_EQ(41u, answers[0].id);
    EXPECT_EQ("", answers[0].data);

    held->answerAll("too-late");

    EXPECT_EQ(R"({"r":"from-below"})", caller.call(43, Requests::named("host.abandoned")).value);
    EXPECT_EQ(0u, xplpc_buffer_register(handle, held.get(), 1));

    xplpc_abandon(&handle);
    xplpc_abandon(nullptr);
}

TEST_F(GeneralTest, LifecycleAbandonReturnsOnlyOnceNothingIsEnteringTheHost)
{
    // A runtime deletes its callbacks right after it abandons its layer, so a post that is still inside the host when abandon returns is a call into code that no longer exists.

    struct State
    {
        std::atomic<bool> gone{false};
        std::atomic<int> entered{0};
        std::atomic<int> late{0};
    };

    auto held = std::make_shared<HeldResponders>();

    // clang-format off
    MappingList::shared()->add("test.held", Map::create<std::string>({}, [held](const Message &, const Response &r) {
        held->add(r);
    }));

    const auto onInvoke = [](void *, xplpc_handle, const char *, size_t) {
    };

    const auto onPost = [](void *context, xplpc_post_kind, std::uint64_t, char *data, size_t) {
        auto state = static_cast<State *>(context);

        xplpc_free(data);
        state->entered++;
        std::this_thread::sleep_for(std::chrono::milliseconds(2));

        if (state->gone)
        {
            state->late++;
        }
    };
    // clang-format on

    State state;
    const xplpc_host host{&state, onInvoke, onPost, true, nullptr, 0};
    const auto layer = xplpc_attach(&host);

    ASSERT_NE(0u, layer);

    for (std::uint64_t token = 0; token < 64; token++)
    {
        xplpc_call_result result{};
        ASSERT_FALSE(xplpc_call(layer, token, Requests::named("test.held").data(), Requests::named("test.held").size(), &result));
    }

    std::vector<std::thread> answering;

    for (size_t worker = 0; worker < 8; worker++)
    {
        // clang-format off
        answering.emplace_back([held, worker] {
            held->answerEvery(worker, 8, "late");
        });
        // clang-format on
    }

    while (state.entered == 0)
    {
        std::this_thread::yield();
    }

    xplpc_abandon(&layer);
    state.gone = true;

    for (auto &thread : answering)
    {
        thread.join();
    }

    EXPECT_EQ(0, state.late.load());
    EXPECT_EQ(0u, xplpc::core::Router::shared().pending());
}

TEST_F(GeneralTest, LifecycleAbandonWaitsForAnInvokeThatIsInsideTheHost)
{
    // A host that can be entered anywhere is invoked directly, and a runtime deletes its callbacks right after it abandons, so an invoke still inside the host has to be waited for.

    struct State
    {
        std::atomic<bool> entered{false};
        std::atomic<bool> leave{false};
    };

    // clang-format off
    const auto onInvoke = [](void *context, xplpc_handle call, const char *, size_t) {
        auto state = static_cast<State *>(context);

        state->entered = true;

        while (!state->leave)
        {
            std::this_thread::yield();
        }

        xplpc_answer(call, "", 0);
    };

    const auto onPost = [](void *, xplpc_post_kind, std::uint64_t, char *data, size_t) {
        xplpc_free(data);
    };
    // clang-format on

    State state;
    const xplpc_host host{&state, onInvoke, onPost, true, nullptr, 0};
    const auto layer = xplpc_attach(&host);

    ASSERT_NE(0u, layer);

    const std::string name = "host.inside";
    xplpc_declare(layer, name.data(), name.size());

    // clang-format off
    std::thread caller([] {
        xplpc::client::Client::call(Requests::named("host.inside"), [](const std::string &) {
        });
    });
    // clang-format on

    while (!state.entered)
    {
        std::this_thread::yield();
    }

    std::atomic<bool> returned{false};

    // clang-format off
    std::thread abandoning([&layer, &returned] {
        xplpc_abandon(&layer);
        returned = true;
    });
    // clang-format on

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_FALSE(returned.load());

    state.leave = true;

    abandoning.join();
    caller.join();

    EXPECT_TRUE(returned.load());
    EXPECT_EQ(0u, xplpc::core::Router::shared().pending());
}
