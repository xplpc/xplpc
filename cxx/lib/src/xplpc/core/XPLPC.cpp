#include "xplpc/core/XPLPC.hpp"
#include "xplpc/data/CallbackList.hpp"
#include "xplpc/util/Log.hpp"

namespace xplpc
{
namespace core
{

std::atomic<bool> XPLPC::initialized{false};

void XPLPC::initialize()
{
    // The logger is published here so a host can redirect this library the moment it is up rather than after the first line it writes.
    util::Log::logger();

    // This says the library can serve a call, so it is the last step of bringing it up rather than one of the first.
    initialized.store(true, std::memory_order_release);
}

void XPLPC::finalize()
{
    initialized.store(false, std::memory_order_release);

    // A call still waiting has nothing left that could ever resolve it, so it is answered with the empty value rather than left pending for the life of the process.
    data::CallbackList::shared()->answerAndClear("");
}

bool XPLPC::isInitialized()
{
    return initialized.load(std::memory_order_acquire);
}

} // namespace core
} // namespace xplpc
