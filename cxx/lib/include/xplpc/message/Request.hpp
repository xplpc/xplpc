#pragma once

#include "xplpc/serializer/Serializer.hpp"

#include <string>

namespace xplpc
{
namespace message
{

using namespace xplpc::serializer;

class Request
{
public:
    template <typename... Args>
    Request(const std::string &functionName, Args &&...paramValues)
        : rawData(Serializer::encodeRequest(functionName, std::forward<Args>(paramValues)...))
        , rawFunctionName(rawData.empty() ? "" : functionName)
    {
    }

    // Answers an empty name for a request that could not be encoded, which routing refuses before a mapping runs.
    const std::string &functionName() const noexcept { return rawFunctionName; }
    const std::string &data() const noexcept { return rawData; }

private:
    const std::string rawData;
    const std::string rawFunctionName;
};

} // namespace message
} // namespace xplpc
