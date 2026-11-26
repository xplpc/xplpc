#pragma once

#include "xplpc/core/Handle.hpp"

#include <string>
#include <utility>

namespace xplpc
{
namespace core
{

// Either the answer the mapping gave before the call returned, or the handle of a call whose answer arrives later through its completion.
class CallResult
{
public:
    static CallResult answered(std::string value)
    {
        return CallResult(0, std::move(value));
    }

    static CallResult pending(Handle call)
    {
        return CallResult(call, "");
    }

    bool isAnswered() const
    {
        return call == 0;
    }

    Handle handle() const
    {
        return call;
    }

    const std::string &value() const
    {
        return answer;
    }

private:
    Handle call;
    std::string answer;

    CallResult(Handle call, std::string answer)
        : call(call)
        , answer(std::move(answer))
    {
    }
};

} // namespace core
} // namespace xplpc
