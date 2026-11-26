#pragma once

#include "xplpc/core/Handle.hpp"
#include "xplpc/core/Router.hpp"

#include <atomic>
#include <coroutine>
#include <functional>
#include <memory>
#include <stdexcept>
#include <utility>

namespace xplpc
{
namespace client
{

// One call awaited once: a named awaitable cannot be awaited at all, and awaiting a moved one raises at the co_await.
template <typename T>
class CallAwaitable
{
public:
    using Answer = std::function<void(T)>;

    // Starts the call, hands an answer it already has to the given function, and answers the handle of a call still waiting.
    using Dispatcher = std::function<core::Handle(const Answer &)>;

    class Awaiter
    {
    public:
        explicit Awaiter(Dispatcher dispatcher)
            : state(std::make_shared<State>())
            , dispatcher(std::move(dispatcher))
        {
        }

        ~Awaiter()
        {
            // The frame owning this is being destroyed, and only a wait that is still suspended has an answer left to disarm.

            auto expected = Status::Suspended;

            if (state->status.compare_exchange_strong(expected, Status::Cancelled, std::memory_order_acq_rel))
            {
                core::Router::shared().cancel(core::Router::shared().native(), call);
            }
        }

        Awaiter(const Awaiter &) = delete;
        Awaiter &operator=(const Awaiter &) = delete;
        Awaiter(Awaiter &&) = delete;
        Awaiter &operator=(Awaiter &&) = delete;

        bool await_ready() const noexcept
        {
            return false;
        }

        bool await_suspend(std::coroutine_handle<> handle)
        {
            // Resuming while this function is still running would destroy the frame it is about to return to, so an answer that arrives here is taken by the return value instead.

            state->handle = handle;

            // clang-format off
            call = dispatcher([sharedState = state](T value) {
                sharedState->complete(std::move(value));
            });
            // clang-format on

            auto expected = Status::Pending;
            return state->status.compare_exchange_strong(expected, Status::Suspended, std::memory_order_acq_rel);
        }

        T await_resume()
        {
            return std::move(state->value);
        }

    private:
        enum class Status
        {
            Pending,
            Answered,
            Suspended,
            Resuming,
            Cancelled,
        };

        struct State
        {
            T value{};
            std::coroutine_handle<> handle;
            std::atomic<Status> status{Status::Pending};

            void complete(T answer)
            {
                value = std::move(answer);

                auto expected = Status::Pending;

                if (status.compare_exchange_strong(expected, Status::Answered, std::memory_order_acq_rel))
                {
                    return;
                }

                expected = Status::Suspended;

                if (status.compare_exchange_strong(expected, Status::Resuming, std::memory_order_acq_rel))
                {
                    handle.resume();
                }
            }
        };

        std::shared_ptr<State> state;
        Dispatcher dispatcher;
        core::Handle call = 0;
    };

    explicit CallAwaitable(Dispatcher dispatcher)
        : dispatcher(std::move(dispatcher))
    {
    }

    CallAwaitable(CallAwaitable &&other) noexcept
        : dispatcher(std::exchange(other.dispatcher, nullptr))
    {
    }

    CallAwaitable &operator=(CallAwaitable &&other) noexcept
    {
        dispatcher = std::exchange(other.dispatcher, nullptr);
        return *this;
    }

    CallAwaitable(const CallAwaitable &) = delete;
    CallAwaitable &operator=(const CallAwaitable &) = delete;

    Awaiter operator co_await() &&
    {
        if (!dispatcher)
        {
            throw std::logic_error("A call can be awaited only once");
        }

        return Awaiter{std::exchange(dispatcher, nullptr)};
    }

private:
    Dispatcher dispatcher;
};

} // namespace client
} // namespace xplpc
