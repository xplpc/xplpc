#pragma once

#include <chrono>
#include <future>

// A mapping is free to answer after the call has returned, so a test waits for the answer instead of assuming it already arrived.
template <typename T>
class AnswerWaiter
{
public:
    AnswerWaiter()
        : future(promise.get_future())
    {
    }

    void answer(const T &value)
    {
        promise.set_value(value);
    }

    bool waitFor(int milliseconds)
    {
        return future.wait_for(std::chrono::milliseconds(milliseconds)) == std::future_status::ready;
    }

    T value()
    {
        return future.get();
    }

private:
    std::promise<T> promise;
    std::future<T> future;
};
