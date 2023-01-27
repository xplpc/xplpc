#pragma once

#include <functional>
#include <string>

#include "xplpc/data/CallbackList.hpp"
#include "xplpc/data/MappingList.hpp"
#include "xplpc/serializer/Serializer.hpp"
#include "xplpc/util/UniqueID.hpp"

#include "spdlog/spdlog.h"

#if defined(__EMSCRIPTEN__)
#include <emscripten/bind.h>
using namespace emscripten;
#endif

namespace xplpc
{
namespace client
{

using namespace xplpc::data;
using namespace xplpc::serializer;
using namespace xplpc::util;

class ProxyClient
{
public:
    ProxyClient() = default;

    static void call(const std::string &data, std::function<void(const std::string &)> callback)
    {
        auto functionName = Serializer::decodeFunctionName(data);

        if (functionName.empty())
        {
            spdlog::error("[ProxyClient : call] Function name is empty");

            if (callback)
            {
                callback("");
            }

            return;
        }

        auto mappingItem = MappingList::shared()->find(functionName);

        if (mappingItem)
        {
            const auto key = UniqueID::generate();

            // clang-format off
            CallbackList::shared()->add(key, [callback](const std::string& response) {
                if (callback)
                {
                    callback(response);
                }
            });
            // clang-format on

            mappingItem.value().getExecutor()(key, data);
        }
        else
        {
            spdlog::error("[ProxyClient : call] Mapping not found for function: {}", functionName);

            if (callback)
            {
                callback("");
            }
        }
    }

    static void callFromPtr(const std::string &data, void (*callbackPtr)(const char *))
    {
        std::function<void(const char *)> callback = static_cast<std::function<void(const char *)>>(callbackPtr);

        auto functionName = Serializer::decodeFunctionName(data);

        if (functionName.empty())
        {
            spdlog::error("[ProxyClient : callFromPtr] Function name is empty");

            if (callback)
            {
                callback("");
            }

            return;
        }

        auto mappingItem = MappingList::shared()->find(functionName);

        if (mappingItem)
        {
            const auto key = UniqueID::generate();

            // clang-format off
            CallbackList::shared()->add(key, [callback](const std::string& response) {
                if (callback)
                {
                    callback(response.c_str());
                }
            });
            // clang-format on

            mappingItem.value().getExecutor()(key, data);
        }
        else
        {
            spdlog::error("[ProxyClient : callFromPtr] Mapping not found for function: {}", functionName);

            if (callback)
            {
                callback("");
            }
        }
    }

#if defined(__EMSCRIPTEN__)
    static void callFromJavascript(const std::string &data, emscripten::val callback)
    {
        auto functionName = Serializer::decodeFunctionName(data);

        if (functionName.empty())
        {
            spdlog::error("[ProxyClient : callFromJavascript] Function name is empty");
            callback(std::string{});
            return;
        }

        auto mappingItem = MappingList::shared()->find(functionName);

        if (mappingItem)
        {
            const auto key = UniqueID::generate();

            // clang-format off
            CallbackList::shared()->add(key, [callback](const std::string& response) {
                callback(response);
            });
            // clang-format on

            mappingItem.value().getExecutor()(key, data);
        }
        else
        {
            spdlog::error("[ProxyClient : callFromJavascript] Mapping not found for function: {}", functionName);
            callback(std::string{});
        }
    }
#endif
};

} // namespace client
} // namespace xplpc
