#pragma once

#include <atomic>
#include <coroutine>
#include <functional>
#include <memory>
#include <optional>
#include <utility>

namespace xplpc
{
namespace client
{

template <typename T>
class CallAwaitable
{
public:
    using Dispatcher = std::function<void(const std::function<void(std::optional<T>)> &)>;

    explicit CallAwaitable(Dispatcher dispatcher)
        : state(std::make_shared<State>())
        , dispatcher(std::move(dispatcher))
    {
    }

    bool await_ready() const noexcept
    {
        return false;
    }

    bool await_suspend(std::coroutine_handle<> handle)
    {
        // Resuming while this function is still running would destroy the frame it is about to return to, so an answer that arrives here is taken by the return value instead.

        auto sharedState = state;
        sharedState->handle = handle;

        // clang-format off
        dispatcher([sharedState](std::optional<T> value) {
            sharedState->value = std::move(value);

            if (sharedState->settled.exchange(true))
            {
                sharedState->handle.resume();
            }
        });
        // clang-format on

        return !sharedState->settled.exchange(true);
    }

    std::optional<T> await_resume()
    {
        return std::move(state->value);
    }

private:
    struct State
    {
        std::optional<T> value;
        std::coroutine_handle<> handle;
        std::atomic<bool> settled{false};
    };

    std::shared_ptr<State> state;
    Dispatcher dispatcher;
};

} // namespace client
} // namespace xplpc
