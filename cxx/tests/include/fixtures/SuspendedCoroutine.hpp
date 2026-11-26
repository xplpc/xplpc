#pragma once

#include "gtest/gtest.h"

#include <coroutine>
#include <utility>

// A coroutine whose frame the test owns, so it can be destroyed while it is still suspended.
class SuspendedCoroutine
{
public:
    struct promise_type
    {
        SuspendedCoroutine get_return_object() { return SuspendedCoroutine{std::coroutine_handle<promise_type>::from_promise(*this)}; }
        std::suspend_never initial_suspend() noexcept { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        void return_void() noexcept {}
        void unhandled_exception() noexcept { ADD_FAILURE() << "the coroutine raised"; }
    };

    explicit SuspendedCoroutine(std::coroutine_handle<promise_type> handle)
        : handle(handle)
    {
    }

    ~SuspendedCoroutine()
    {
        destroy();
    }

    SuspendedCoroutine(const SuspendedCoroutine &) = delete;
    SuspendedCoroutine &operator=(const SuspendedCoroutine &) = delete;

    bool done() const { return handle.done(); }

    void destroy()
    {
        if (handle)
        {
            std::exchange(handle, nullptr).destroy();
        }
    }

private:
    std::coroutine_handle<promise_type> handle;
};
