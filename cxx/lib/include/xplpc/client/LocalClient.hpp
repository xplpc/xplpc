#pragma once

#include <exception>
#include <functional>
#include <optional>
#include <string>

#include "xplpc/data/MappingList.hpp"
#include "xplpc/message/Request.hpp"
#include "xplpc/serializer/Serializer.hpp"
#include "xplpc/util/UniqueID.hpp"

#include "spdlog/spdlog.h"

namespace xplpc
{
namespace client
{

using namespace xplpc::data;
using namespace xplpc::message;
using namespace xplpc::serializer;
using namespace xplpc::util;

class LocalClient
{
public:
    LocalClient() = default;

    template <typename T>
    static void call(const Request &request, std::function<void(const std::optional<T> &)> callback)
    {
        auto functionName = Serializer::decodeFunctionName(request.data());

        if (functionName.empty())
        {
            spdlog::error("[LocalClient : call] Function name is empty");

            if (callback)
            {
                callback(std::nullopt);
            }

            return;
        }

        auto mappingItem = MappingList::shared()->find(functionName);

        if (mappingItem)
        {
            const auto key = UniqueID::generate();

            // clang-format off
            CallbackList::shared()->add(key, [callback, functionName](const std::string& response) {
                std::optional<T> decodedData;

                try
                {
                    decodedData = Serializer::decodeFunctionReturnValue<T>(response);
                }
                catch (const std::exception &e)
                {
                    spdlog::error("[LocalClient : call] Error when decode data: {}", e.what());
                    decodedData = std::nullopt;
                }

                if (callback)
                {
                    callback(decodedData);
                }
            });
            // clang-format on

            mappingItem.value().getExecutor()(key, request.data());
        }
        else
        {
            spdlog::error("[LocalClient : call] Mapping not found for function: {}", functionName);

            if (callback)
            {
                callback(std::nullopt);
            }
        }
    }
};

} // namespace client
} // namespace xplpc
