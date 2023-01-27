#pragma once

#include <optional>
#include <string>
#include <utility>

#include "xplpc/serializer/Serializer.hpp"

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
    {
        rawData = Serializer::encodeRequest(functionName, std::forward<Args>(paramValues)...);
    }

    const std::string &data() const { return this->rawData; }

private:
    std::string rawData;
};

} // namespace message
} // namespace xplpc
