#include "fixtures/CompletionRecorder.hpp"
#include "fixtures/GeneralTest.hpp"
#include "fixtures/HeldCalls.hpp"
#include "fixtures/LogRecorder.hpp"
#include "fixtures/ReenteringSink.hpp"
#include "fixtures/ScopedLayer.hpp"
#include "fixtures/SilentMapping.hpp"
#include "fixtures/SpyMapping.hpp"
#include "xplpc/core/ThreadScope.hpp"
#include "xplpc/xplpc.hpp"
#include "gtest/gtest.h"

#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <future>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

using namespace xplpc::client;
using namespace xplpc::core;
using namespace xplpc::message;

TEST_F(GeneralTest, RouterAnswersWithTheValueOfAnInlineAnswer)
{
    ScopedLayer host(ScopedLayer::answering("inline"));
    host.declare("test.router");

    CompletionRecorder recorder;
    const auto result = Router::shared().call(Router::shared().native(), "test.router", "{}", recorder.completion());

    ASSERT_TRUE(result.isAnswered());
    EXPECT_EQ(R"({"r":"inline"})", result.value());
    EXPECT_EQ(0u, result.handle());
    EXPECT_EQ(0u, recorder.count());
}

TEST_F(GeneralTest, RouterAnswersALateAnswerThroughTheCompletionOnce)
{
    HeldCalls held;
    ScopedLayer host(held.behaviour());
    host.declare("test.router");

    CompletionRecorder recorder;
    const auto result = Router::shared().call(Router::shared().native(), "test.router", "{}", recorder.completion());

    ASSERT_FALSE(result.isAnswered());
    ASSERT_EQ(std::vector<Handle>{result.handle()}, held.all());
    EXPECT_EQ(0u, recorder.count());
    EXPECT_EQ(1u, Router::shared().pending());

    Router::shared().answer(result.handle(), R"({"r":"late"})");
    Router::shared().answer(result.handle(), R"({"r":"again"})");

    EXPECT_EQ(std::vector<std::string>{R"({"r":"late"})"}, recorder.responses());
    EXPECT_EQ(0u, Router::shared().pending());
}

TEST_F(GeneralTest, RouterDropsAnAnswerToACancelledCallWithoutReporting)
{
    HeldCalls held;
    ScopedLayer host(held.behaviour());
    host.declare("test.router");

    CompletionRecorder recorder;
    const auto result = Router::shared().call(Router::shared().native(), "test.router", "{}", recorder.completion());

    ASSERT_FALSE(result.isAnswered());
    EXPECT_TRUE(Router::shared().cancel(Router::shared().native(), result.handle()));
    EXPECT_EQ(0u, Router::shared().pending());

    LogRecorder log;
    Router::shared().answer(result.handle(), R"({"r":"late"})");

    EXPECT_EQ(0u, recorder.count());
    EXPECT_TRUE(log.reportedNothing());
    EXPECT_FALSE(Router::shared().cancel(Router::shared().native(), result.handle()));
}

TEST_F(GeneralTest, RouterRefusesACancelFromALayerThatDidNotMakeTheCall)
{
    HeldCalls held;
    ScopedLayer host(held.behaviour());
    host.declare("test.router");

    CompletionRecorder recorder;
    const auto result = Router::shared().call(Router::shared().native(), "test.router", "{}", recorder.completion());

    ASSERT_FALSE(result.isAnswered());

    LogRecorder log;

    EXPECT_FALSE(Router::shared().cancel(host.get(), result.handle()));
    EXPECT_FALSE(log.reportedNothing());
    EXPECT_EQ(1u, Router::shared().pending());

    Router::shared().answer(result.handle(), R"({"r":"late"})");

    EXPECT_EQ(1u, recorder.count());
}

TEST_F(GeneralTest, RouterRefusesToCancelACallAlreadyAnswered)
{
    HeldCalls held;
    ScopedLayer host(held.behaviour());
    host.declare("test.router");

    CompletionRecorder recorder;
    const auto result = Router::shared().call(Router::shared().native(), "test.router", "{}", recorder.completion());

    ASSERT_FALSE(result.isAnswered());
    Router::shared().answer(result.handle(), R"({"r":"late"})");

    EXPECT_FALSE(Router::shared().cancel(Router::shared().native(), result.handle()));
    EXPECT_EQ(1u, recorder.count());
}

TEST_F(GeneralTest, RouterLetsACancelledCallGoOutsideItsLock)
{
    // Destroying a completion can release what a host holds and reach back into the router, which deadlocks if the call is let go under the lock.

    HeldCalls held;
    ScopedLayer host(held.behaviour());
    host.declare("test.router");

    // clang-format off
    auto reachesTheRouter = std::shared_ptr<int>(new int(0), [](int *value) {
        Router::shared().pending();
        delete value;
    });

    const auto result = Router::shared().call(Router::shared().native(), "test.router", "{}", [reachesTheRouter](const std::string &) {});
    // clang-format on

    reachesTheRouter.reset();
    ASSERT_FALSE(result.isAnswered());

    std::promise<bool> cancelled;
    auto finished = cancelled.get_future();

    // clang-format off
    std::thread cancelling([&cancelled, &result]() {
        cancelled.set_value(Router::shared().cancel(Router::shared().native(), result.handle()));
    });
    // clang-format on

    if (finished.wait_for(std::chrono::seconds(5)) != std::future_status::ready)
    {
        // The router stays locked for good, so no later test could run and the process ends here rather than hanging.
        std::fprintf(stderr, "Cancelling a call deadlocked on the router lock\n");
        std::_Exit(EXIT_FAILURE);
    }

    cancelling.join();

    EXPECT_TRUE(finished.get());
}

TEST_F(GeneralTest, RouterAnswersEmptyOnceWhenALayerRaises)
{
    // clang-format off
    ScopedLayer raisingException([](FakeLayer &, Handle, const std::string &, const std::string &) {
        throw std::runtime_error("the host raised");
    });

    ScopedLayer raisingSomethingElse([](FakeLayer &, Handle, const std::string &, const std::string &) {
        throw 42;
    });
    // clang-format on

    raisingException.declare("test.raises.exception");
    raisingSomethingElse.declare("test.raises.other");

    for (const auto *name : {"test.raises.exception", "test.raises.other"})
    {
        CompletionRecorder recorder;
        std::optional<CallResult> result;

        EXPECT_NO_THROW(result = Router::shared().call(Router::shared().native(), name, "{}", recorder.completion())) << name;

        ASSERT_TRUE(result.has_value()) << name;
        ASSERT_TRUE(result->isAnswered()) << name;
        EXPECT_EQ("", result->value()) << name;
        EXPECT_EQ(0u, recorder.count()) << name;
    }
}

TEST_F(GeneralTest, RouterKeepsTheAnswerOfALayerThatRaisesAfterAnswering)
{
    // The layer already answered when it raised, so answering again would reach the caller a second time.

    // clang-format off
    ScopedLayer host([](FakeLayer &, Handle call, const std::string &, const std::string &) {
        Router::shared().answer(call, R"({"r":"answered"})");
        throw std::runtime_error("the host raised after answering");
    });
    // clang-format on

    host.declare("sample.echo");

    auto answers = std::make_shared<std::vector<std::optional<std::string>>>();

    // clang-format off
    EXPECT_NO_THROW(Client::call<std::string>(Request{"sample.echo", Param<std::string>{"value", "x"}}, [answers](const std::optional<std::string> &response) {
        answers->push_back(response);
    }));
    // clang-format on

    ASSERT_EQ(1u, answers->size());
    EXPECT_EQ("answered", answers->front().value_or(""));
}

TEST_F(GeneralTest, ClientAnswersOnceWhenALayerRaisesBeforeAnswering)
{
    // clang-format off
    ScopedLayer host([](FakeLayer &, Handle, const std::string &, const std::string &) {
        throw std::runtime_error("the host raised");
    });
    // clang-format on

    host.declare("sample.echo");

    auto answers = std::make_shared<std::vector<std::optional<std::string>>>();

    // clang-format off
    EXPECT_NO_THROW(Client::call<std::string>(Request{"sample.echo", Param<std::string>{"value", "x"}}, [answers](const std::optional<std::string> &response) {
        answers->push_back(response);
    }));
    // clang-format on

    ASSERT_EQ(1u, answers->size());
    EXPECT_EQ(std::nullopt, answers->front());
}

TEST_F(GeneralTest, RouterAnswersEmptyForAFunctionNobodyOwns)
{
    LogRecorder log;
    CompletionRecorder recorder;

    const auto result = Router::shared().call(Router::shared().native(), "test.nobody", "{}", recorder.completion());

    ASSERT_TRUE(result.isAnswered());
    EXPECT_EQ("", result.value());
    EXPECT_EQ(0u, recorder.count());

    const auto errors = log.errors();

    ASSERT_EQ(1u, errors.size());
    EXPECT_NE(std::string::npos, errors.front().find("Function not found"));
}

TEST_F(GeneralTest, RouterAnswersEmptyToACallerThatIsNotAttached)
{
    // A caller that is not attached could never be answered later, so the call is refused before any mapping runs.

    SpyMapping::add();

    ScopedLayer gone(ScopedLayer::answering("gone"));
    gone.detach();

    for (const auto caller : {Handle{0}, gone.get()})
    {
        CompletionRecorder recorder;
        const auto result = Router::shared().call(caller, SpyMapping::name, R"({"f":"test.spy","p":[]})", recorder.completion());

        ASSERT_TRUE(result.isAnswered());
        EXPECT_EQ("", result.value());
        EXPECT_EQ(0u, recorder.count());
    }

    EXPECT_EQ(0, SpyMapping::count());
}

TEST_F(GeneralTest, RouterAnswersEveryCallADetachedLayerWasServing)
{
    HeldCalls held;
    ScopedLayer host(held.behaviour());
    host.declare("test.held");

    auto answers = std::make_shared<std::vector<std::optional<std::string>>>();

    for (int i = 0; i < 2; i++)
    {
        // clang-format off
        Client::call<std::string>(Request{"test.held"}, [answers](const std::optional<std::string> &response) {
            answers->push_back(response);
        });
        // clang-format on
    }

    ASSERT_EQ(2u, Router::shared().pending());
    EXPECT_TRUE(answers->empty());

    host.detach();

    ASSERT_EQ(2u, answers->size());
    EXPECT_EQ(std::nullopt, answers->at(0));
    EXPECT_EQ(std::nullopt, answers->at(1));
    EXPECT_EQ(0u, Router::shared().pending());

    // The layer answering after it detached reaches nobody, since the calls were already answered.
    for (const auto call : held.all())
    {
        Router::shared().answer(call, R"({"r":"late"})");
    }

    EXPECT_EQ(2u, answers->size());
}

TEST_F(GeneralTest, RouterAnswersEveryCallADetachedLayerMade)
{
    SilentMapping::add();

    ScopedLayer caller(ScopedLayer::answering("unused"));
    CompletionRecorder recorder;

    const auto result = Router::shared().call(caller.get(), SilentMapping::name, R"({"f":"test.never","p":[]})", recorder.completion());

    ASSERT_FALSE(result.isAnswered());
    EXPECT_EQ(1u, Router::shared().pending());

    caller.detach();

    EXPECT_EQ(std::vector<std::string>{""}, recorder.responses());
    EXPECT_EQ(0u, Router::shared().pending());
}

TEST_F(GeneralTest, RouterAnswersTheOthersWhenOneCompletionRaisesDuringDetach)
{
    // Every call was already taken when the first completion raises, so stopping there would leave the rest unanswered for good.

    HeldCalls held;
    ScopedLayer host(held.behaviour());
    host.declare("test.held");

    auto answered = std::make_shared<std::atomic<int>>(0);

    // clang-format off
    Router::shared().call(Router::shared().native(), "test.held", "{}", [](const std::string &) {
        throw std::runtime_error("the consumer raised");
    });

    for (int i = 0; i < 2; i++)
    {
        Router::shared().call(Router::shared().native(), "test.held", "{}", [answered](const std::string &) {
            answered->fetch_add(1);
        });
    }
    // clang-format on

    ASSERT_EQ(3u, Router::shared().pending());

    EXPECT_NO_THROW(host.detach());

    EXPECT_EQ(2, answered->load());
    EXPECT_EQ(0u, Router::shared().pending());
}

TEST_F(GeneralTest, RouterAnsweringACompletionThatRaisesDoesNotRaise)
{
    // An answer is often delivered from a thread of its own, where a raise reaching the top ends the process.

    HeldCalls held;
    ScopedLayer host(held.behaviour());
    host.declare("test.held");

    // clang-format off
    const auto result = Router::shared().call(Router::shared().native(), "test.held", "{}", [](const std::string &) {
        throw 42;
    });
    // clang-format on

    ASSERT_FALSE(result.isAnswered());
    EXPECT_NO_THROW(Router::shared().answer(result.handle(), R"({"r":"late"})"));
    EXPECT_EQ(0u, Router::shared().pending());
}

TEST_F(GeneralTest, RouterDestroysALayerThatDetachesInsideItsOwnInvokeOnlyOnceItReturns)
{
    auto destroyed = std::make_shared<std::atomic<bool>>(false);
    auto destroyedInside = std::make_shared<std::atomic<bool>>(true);

    // clang-format off
    ScopedLayer host([destroyed, destroyedInside](FakeLayer &layer, Handle call, const std::string &, const std::string &) {
        Router::shared().detach(layer.handle.load());

        // The detach already answered this call, so the answer below reaches nobody, and the layer is still alive to make it.
        destroyedInside->store(destroyed->load());
        Router::shared().answer(call, R"({"r":"after"})");
    }, destroyed);
    // clang-format on

    host.declare("test.self.detach");

    CompletionRecorder recorder;
    const auto result = Router::shared().call(Router::shared().native(), "test.self.detach", "{}", recorder.completion());

    EXPECT_FALSE(destroyedInside->load());
    EXPECT_TRUE(destroyed->load());
    EXPECT_EQ(nullptr, Router::shared().layer(host.get()));

    ASSERT_TRUE(result.isAnswered());
    EXPECT_EQ("", result.value());
    EXPECT_EQ(0u, recorder.count());
}

TEST_F(GeneralTest, RouterDetachReturnsWithoutWaitingForACallInsideTheLayer)
{
    // The layer is released when the last invocation inside it leaves, and the detach itself does not wait for that.

    auto destroyed = std::make_shared<std::atomic<bool>>(false);
    auto entered = std::make_shared<std::atomic<bool>>(false);
    auto leave = std::make_shared<std::atomic<bool>>(false);

    // clang-format off
    ScopedLayer host([entered, leave](FakeLayer &, Handle, const std::string &, const std::string &) {
        entered->store(true);

        while (!leave->load())
        {
            std::this_thread::yield();
        }
    }, destroyed);
    // clang-format on

    host.declare("test.inside");

    std::optional<CallResult> result;

    // clang-format off
    std::thread calling([&result]() {
        result = Router::shared().call(Router::shared().native(), "test.inside", "{}", nullptr);
    });
    // clang-format on

    while (!entered->load())
    {
        std::this_thread::yield();
    }

    host.detach();

    EXPECT_FALSE(destroyed->load());

    leave->store(true);
    calling.join();

    EXPECT_TRUE(destroyed->load());

    ASSERT_TRUE(result.has_value());
    ASSERT_TRUE(result->isAnswered());
    EXPECT_EQ("", result->value());
}

TEST_F(GeneralTest, RouterHoldsNoLockWhileALayerServesACall)
{
    // A layer is free to attach, declare, call and detach another from inside its own invoke.

    // clang-format off
    ScopedLayer outer([](FakeLayer &layer, Handle call, const std::string &, const std::string &) {
        ScopedLayer inner(ScopedLayer::answering("inner"));
        inner.declare("test.inner");

        const auto nested = Router::shared().call(layer.handle.load(), "test.inner", "{}", nullptr);

        inner.detach();
        Router::shared().answer(call, nested.value());
    });
    // clang-format on

    outer.declare("test.outer");

    const auto result = Router::shared().call(Router::shared().native(), "test.outer", "{}", nullptr);

    ASSERT_TRUE(result.isAnswered());
    EXPECT_EQ(R"({"r":"inner"})", result.value());
}

TEST_F(GeneralTest, RouterEntersALayerInsideTheScopeOfItsCaller)
{
    // The thread scope names the layer that started the call, which is what lets a host tell its own thread from any other.

    auto insideCaller = std::make_shared<std::atomic<bool>>(false);
    auto insideItself = std::make_shared<std::atomic<bool>>(true);

    // clang-format off
    ScopedLayer host([insideCaller, insideItself](FakeLayer &layer, Handle call, const std::string &, const std::string &) {
        insideCaller->store(ThreadScope::inside(Router::shared().native()));
        insideItself->store(ThreadScope::inside(layer.handle.load()));
        Router::shared().answer(call, "");
    });
    // clang-format on

    host.declare("test.scope");
    Router::shared().call(Router::shared().native(), "test.scope", "{}", nullptr);

    EXPECT_TRUE(insideCaller->load());
    EXPECT_FALSE(insideItself->load());
    EXPECT_FALSE(ThreadScope::inside(Router::shared().native()));
}

TEST(ThreadScopeTest, ScopesNestAndStayOnTheirOwnThread)
{
    constexpr Handle first = 101;
    constexpr Handle second = 202;

    EXPECT_FALSE(ThreadScope::inside(first));

    {
        ThreadScope outer(first);

        EXPECT_TRUE(ThreadScope::inside(first));
        EXPECT_FALSE(ThreadScope::inside(second));

        {
            ThreadScope inner(second);

            EXPECT_TRUE(ThreadScope::inside(first));
            EXPECT_TRUE(ThreadScope::inside(second));

            auto elsewhere = true;

            // clang-format off
            std::thread([&elsewhere]() {
                elsewhere = ThreadScope::inside(first) || ThreadScope::inside(second);
            }).join();
            // clang-format on

            EXPECT_FALSE(elsewhere);
        }

        EXPECT_TRUE(ThreadScope::inside(first));
        EXPECT_FALSE(ThreadScope::inside(second));
    }

    EXPECT_FALSE(ThreadScope::inside(first));
}

TEST_F(GeneralTest, StressRouterAnswersEveryCallOnceWhileLayersComeAndGo)
{
    // Calls are allocated, answered from workers, cancelled and orphaned by detaching layers all at once, and none of them may resolve twice or stay waiting.

    constexpr int callers = 8;
    constexpr int iterations = 300;
    constexpr int workers = 2;

    struct Record
    {
        std::shared_ptr<std::atomic<int>> runs;
        bool answeredInline;
        bool cancelled;
    };

    HeldCalls held;
    ScopedLayer deferred(held.behaviour());
    deferred.declare("test.stress");

    std::atomic<bool> calling{true};
    std::atomic<bool> answering{true};
    std::vector<std::vector<Record>> records(callers + 1);

    // clang-format off
    const auto place = [](Handle caller, std::vector<Record> &into, bool cancel) {
        auto runs = std::make_shared<std::atomic<int>>(0);

        const auto result = Router::shared().call(caller, "test.stress", "{}", [runs](const std::string &) {
            runs->fetch_add(1);
        });

        const auto cancelled = !result.isAnswered() && cancel && Router::shared().cancel(caller, result.handle());
        into.push_back(Record{runs, result.isAnswered(), cancelled});
    };
    // clang-format on

    std::vector<std::thread> answerers;

    for (int w = 0; w < workers; w++)
    {
        // clang-format off
        answerers.emplace_back([&held, &answering]() {
            while (answering.load())
            {
                const auto call = held.next();

                if (!call)
                {
                    std::this_thread::yield();
                    continue;
                }

                Router::shared().answer(call.value(), R"({"r":"late"})");
            }
        });
        // clang-format on
    }

    // clang-format off
    std::thread churn([&calling, &place, &records]() {
        for (int k = 0; calling.load(); k++)
        {
            ScopedLayer other(ScopedLayer::answering("other"));

            if (k % 3 == 0)
            {
                other.declare("test.stress");
            }

            place(other.get(), records[callers], false);
        }
    });
    // clang-format on

    std::vector<std::thread> threads;

    for (int t = 0; t < callers; t++)
    {
        // clang-format off
        threads.emplace_back([t, &place, &records]() {
            for (int i = 0; i < iterations; i++)
            {
                place(Router::shared().native(), records[t], i % 2 == 0);
            }
        });
        // clang-format on
    }

    for (auto &thread : threads)
    {
        thread.join();
    }

    calling.store(false);
    churn.join();

    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(30);

    while (Router::shared().pending() > 0 && std::chrono::steady_clock::now() < deadline)
    {
        std::this_thread::yield();
    }

    answering.store(false);

    for (auto &answerer : answerers)
    {
        answerer.join();
    }

    EXPECT_EQ(0u, Router::shared().pending());

    int late = 0;
    int cancelled = 0;

    for (const auto &list : records)
    {
        for (const auto &record : list)
        {
            const auto runs = record.runs->load();

            if (record.answeredInline || record.cancelled)
            {
                EXPECT_EQ(0, runs);
            }
            else
            {
                EXPECT_EQ(1, runs);
                late++;
            }

            cancelled += record.cancelled ? 1 : 0;
        }
    }

    // A run that never reached either path would prove nothing about the race between them.
    EXPECT_GT(late, 0);
    EXPECT_GT(cancelled, 0);
}

TEST_F(GeneralTest, RouterRefusesToAttachALayerThatDoesNotExist)
{
    // A layer that is not there would be dereferenced the first time a call is routed to a name it declared.

    LogRecorder recorder;

    EXPECT_EQ(0u, Router::shared().attach(nullptr));
    EXPECT_EQ(1u, recorder.errors().size());
}

TEST_F(GeneralTest, RouterReportsEveryRefusalWithItsLockFree)
{
    // A sink that reads the router from the line it is handed would wait forever on a lock the reporting thread already holds.

    HeldCalls held;
    ScopedLayer host(held.behaviour());
    host.declare("test.router");

    ScopedLayer gone(ScopedLayer::answering("gone"));
    gone.detach();

    CompletionRecorder recorder;
    const auto result = Router::shared().call(Router::shared().native(), "test.router", "{}", recorder.completion());

    ASSERT_FALSE(result.isAnswered());

    // clang-format off
    ReenteringSink sink([] {
        Router::shared().pending();
    });
    // clang-format on

    EXPECT_TRUE(Router::shared().call(Router::shared().native(), "test.nobody", "{}", recorder.completion()).isAnswered());
    EXPECT_TRUE(Router::shared().call(gone.get(), "test.router", "{}", recorder.completion()).isAnswered());
    EXPECT_FALSE(Router::shared().cancel(host.get(), result.handle()));
    Router::shared().declare(gone.get(), "test.gone");

    EXPECT_EQ(4, sink.count());

    Router::shared().answer(result.handle(), R"({"r":"late"})");

    EXPECT_EQ(1u, recorder.count());
}
