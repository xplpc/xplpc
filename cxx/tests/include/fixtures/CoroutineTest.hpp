#pragma once

#include <coroutine>
#include <exception>
#include <future>
#include <memory>
#include <utility>

// A test body that awaits has to be a coroutine, and the test thread waits here for it to finish.
class CoroutineTest
{
public:
    struct promise_type
    {
        std::shared_ptr<std::promise<void>> finished = std::make_shared<std::promise<void>>();
        std::exception_ptr error;

        CoroutineTest get_return_object() { return CoroutineTest{finished}; }
        std::suspend_never initial_suspend() noexcept { return {}; }

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

    explicit CoroutineTest(std::shared_ptr<std::promise<void>> finished)
        : finished(std::move(finished))
    {
    }

    void wait() { finished->get_future().get(); }

private:
    std::shared_ptr<std::promise<void>> finished;
};
