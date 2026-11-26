#include "xplpc/core/CallState.hpp"
#include "xplpc/util/Log.hpp"

#include <exception>
#include <utility>

namespace xplpc
{
namespace core
{

CallState::CallState(Completion completion)
    : completion(std::move(completion))
{
}

void CallState::resolve(std::string response)
{
    // An answer that arrives while the call is still being dispatched is handed back as its value, so the completion only ever sees a late one.

    value = std::move(response);

    auto expected = Status::Dispatching;

    if (status.compare_exchange_strong(expected, Status::Resolved, std::memory_order_acq_rel))
    {
        return;
    }

    run(completion, value);
}

bool CallState::settle()
{
    auto expected = Status::Dispatching;
    return status.compare_exchange_strong(expected, Status::Pending, std::memory_order_acq_rel);
}

std::string CallState::takeValue()
{
    return std::move(value);
}

void CallState::run(const Completion &completion, const std::string &response)
{
    // A late answer is often delivered on a thread of its own, where a raise has nothing above it but the end of the process.

    if (!completion)
    {
        return;
    }

    try
    {
        completion(response);
    }
    catch (const std::exception &e)
    {
        util::Log::e("[CallState : run] The callback raised while being answered");
        util::Log::d("[CallState : run] The callback raised while being answered: {}", e.what());
    }
    catch (...)
    {
        util::Log::e("[CallState : run] The callback raised something that is not an exception while being answered");
    }
}

} // namespace core
} // namespace xplpc
