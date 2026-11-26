#pragma once

#include <atomic>
#include <functional>
#include <string>

namespace xplpc
{
namespace core
{

using Completion = std::function<void(const std::string &)>;

// The one place a call is resolved, which decides whether the answer goes back as the value of the call or through its completion.
class CallState
{
public:
    explicit CallState(Completion completion);

    // Runs once per call, since only whoever took the call out of the router reaches it.
    void resolve(std::string response);

    // Answers true when the call is still waiting once the dispatch returned, and false when it was already answered.
    bool settle();

    std::string takeValue();

    // Hands an answer to a completion, and a completion that raises is reported rather than left to end the thread it runs on.
    static void run(const Completion &completion, const std::string &response);

private:
    enum class Status
    {
        Dispatching,
        Pending,
        Resolved,
    };

    std::atomic<Status> status{Status::Dispatching};
    std::string value;
    Completion completion;
};

} // namespace core
} // namespace xplpc
