#pragma once

#include <future>
#include <utility>

#include "xplpc/data/MappingData.hpp"
#include "xplpc/serializer/Serializer.hpp"

#include "spdlog/spdlog.h"

namespace xplpc
{
namespace client
{

using namespace xplpc::serializer;
using namespace xplpc::data;

class ProxyClient
{
public:
    ProxyClient() = default;

    static std::string call(const std::string &data)
    {
        auto functionName = Serializer::decodeFunctionName(data);

        if (functionName.empty())
        {
            spdlog::error("[ProxyClient : call] Function name is empty");
            return "";
        }

        auto mappingItem = MappingData::find(functionName);

        if (mappingItem)
        {
            return mappingItem.value().getExecutor()(data);
        }

        spdlog::error("[ProxyClient : call] Mapping not found for function: {}", functionName);

        return "";
    }

    static std::future<std::string> callAsync(const std::string &data)
    {
        return std::async([&]()
                          { return call(data); });
    }
};

} // namespace client
} // namespace xplpc
