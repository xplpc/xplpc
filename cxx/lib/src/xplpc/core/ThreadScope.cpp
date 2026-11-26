#include "xplpc/core/ThreadScope.hpp"

namespace xplpc
{
namespace core
{

thread_local const ThreadScope *ThreadScope::current = nullptr;

ThreadScope::ThreadScope(Handle layer)
    : layer(layer)
    , previous(current)
{
    current = this;
}

ThreadScope::~ThreadScope()
{
    current = previous;
}

bool ThreadScope::inside(Handle layer)
{
    for (auto scope = current; scope; scope = scope->previous)
    {
        if (scope->layer == layer)
        {
            return true;
        }
    }

    return false;
}

} // namespace core
} // namespace xplpc
