#include "fixtures/CoroutineTest.hpp"
#include "fixtures/GeneralTest.hpp"
#include "fixtures/SuspendedCoroutine.hpp"
#include "xplpc/xplpc.hpp"
#include "gtest/gtest.h"

#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

using namespace xplpc::client;
using namespace xplpc::data;
using namespace xplpc::map;
using namespace xplpc::message;

// Awaiting a named awaitable would let a second wait share the first one's state, so it has to fail to compile.
template <typename T>
concept AwaitableByName = requires(T &awaitable) { awaitable.operator co_await(); };

TEST_F(GeneralTest, AwaitableIsNeverResumedAfterItsCoroutineWasDestroyed)
{
    // The answer holds the wait, not the frame, so a frame destroyed while suspended has to disarm the answer itself.

    auto responder = std::make_shared<Response>();

    // clang-format off
    MappingList::shared()->add("test.later", Map::create<std::string>({}, [responder](const Message &, const Response &r) {
        *responder = r;
    }));
    // clang-format on

    auto resumed = std::make_shared<bool>(false);

    // clang-format off
    auto coroutine = [](std::shared_ptr<bool> flag) -> SuspendedCoroutine {
        co_await Client::callAsync<std::string>(Request{"test.later"});
        *flag = true;
    }(resumed);
    // clang-format on

    ASSERT_FALSE(coroutine.done());
    ASSERT_TRUE(*responder);
    EXPECT_EQ(1u, Router::shared().pending());

    coroutine.destroy();

    EXPECT_EQ(0u, Router::shared().pending());

    (*responder)(std::string{"late"});

    EXPECT_FALSE(*resumed);
}

TEST_F(GeneralTest, AwaitableCannotBeAwaitedTwice)
{
    // A second wait on the same call would share the first one's state, so it is refused at the co_await rather than answering a default value.

    static_assert(!std::is_copy_constructible_v<CallAwaitable<std::optional<std::string>>>);
    static_assert(!AwaitableByName<CallAwaitable<std::optional<std::string>>>);

    // clang-format off
    auto task = []() -> CoroutineTest {
        auto awaitable = Client::callAsync<std::string>(Request{
            "sample.login",
            Param<std::string>{"username", "paulo"},
            Param<std::string>{"password", "123456"},
            Param<bool>{"remember", true},
        });

        auto first = co_await std::move(awaitable);
        EXPECT_EQ("LOGGED-WITH-REMEMBER", first.value_or(""));

        auto refused = false;

        try
        {
            co_await std::move(awaitable);
        }
        catch (const std::logic_error &)
        {
            refused = true;
        }

        EXPECT_TRUE(refused);
    }();
    // clang-format on

    task.wait();
}

TEST_F(GeneralTest, AwaitableKeepsTheValueOfAnAnswerThatArrivesBeforeItSuspends)
{
    // A mapping answering inline completes the wait inside await_suspend, which is the path where resuming would destroy the frame being returned to.

    // clang-format off
    auto task = []() -> CoroutineTest {
        auto response = co_await Client::callAsync<std::string>(Request{"sample.echo", Param<std::string>{"value", "inline"}});
        EXPECT_EQ("inline", response.value_or(""));
    }();
    // clang-format on

    task.wait();
}
