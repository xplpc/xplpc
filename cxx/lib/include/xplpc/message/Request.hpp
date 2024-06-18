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
        : rawFunctionName(functionName)
        , rawData(Serializer::encodeRequest(functionName, std::forward<Args>(paramValues)...))
    {
    }

    const std::string &functionName() const noexcept { return rawFunctionName; }
    const std::string &data() const noexcept { return rawData; }

private:
    const std::string rawData;
    const std::string rawFunctionName;
};

} // namespace message
} // namespace xplpc
