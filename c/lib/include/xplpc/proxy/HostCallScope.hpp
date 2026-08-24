#pragma once

namespace xplpc
{
namespace proxy
{

// The thread is marked for as long as it runs a call the host started, which is the only thread the host can be answered on directly.
class HostCallScope
{
public:
    HostCallScope() { depth++; }
    ~HostCallScope() { depth--; }

    HostCallScope(const HostCallScope &) = delete;
    HostCallScope &operator=(const HostCallScope &) = delete;

    static bool active() { return depth > 0; }

private:
    static thread_local int depth;
};

} // namespace proxy
} // namespace xplpc
