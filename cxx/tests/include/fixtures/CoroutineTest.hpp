#pragma once

#include <coroutine>
#include <exception>
#include <future>
#include <memory>
#include <utility>

// A test body that awaits has to be a coroutine, and the test thread runs it and waits here for it to finish.
// It starts suspended, since gcc touches the frame of a coroutine that started on its own after the first suspension, which races with an answer resuming it from another thread.
class CoroutineTest
{
public:
    struct promise_type
    {
        std::shared_ptr<std::promise<void>> finished = std::make_shared<std::promise<void>>();
        std::exception_ptr error;

        CoroutineTest get_return_object() { return CoroutineTest{std::coroutine_handle<promise_type>::from_promise(*this), finished}; }
        std::suspend_always initial_suspend() noexcept { return {}; }

        std::suspend_never final_suspend() noexcept
        {
            if (error)
            {
                finished->set_exception(error);
            }
            else
            {
                finished->set_value();
            }

            return {};
        }

        void return_void() noexcept {}
        void unhandled_exception() noexcept { error = std::current_exception(); }
    };

    CoroutineTest(std::coroutine_handle<promise_type> handle, std::shared_ptr<std::promise<void>> finished)
        : handle(handle)
        , finished(std::move(finished))
    {
    }

    void wait()
    {
        // The frame destroys itself when the body ends, so the handle is resumed once and never touched again.
        auto future = finished->get_future();
        std::exchange(handle, nullptr).resume();
        future.get();
    }

private:
    std::coroutine_handle<promise_type> handle;
    std::shared_ptr<std::promise<void>> finished;
};
