#pragma once

#include "xplpc/core/Handle.hpp"

namespace xplpc
{
namespace core
{

// Marks the current thread as running a call the given layer started, for as long as the scope lives.
class ThreadScope
{
public:
    explicit ThreadScope(Handle layer);
    ~ThreadScope();

    ThreadScope(const ThreadScope &) = delete;
    ThreadScope &operator=(const ThreadScope &) = delete;

    // Answers whether a call the layer started is running anywhere on the current thread, which is the only thread some hosts can be entered on.
    static bool inside(Handle layer);

private:
    Handle layer;
    const ThreadScope *previous;

    static thread_local const ThreadScope *current;
};

} // namespace core
} // namespace xplpc
