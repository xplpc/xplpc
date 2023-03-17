#pragma once

#include <exception>
#include <functional>
#include <optional>
#include <string>

#include "xplpc/data/CallbackList.hpp"
#include "xplpc/data/PlatformProxyList.hpp"
#include "xplpc/message/Request.hpp"
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
using namespace xplpc::message;
using namespace xplpc::proxy;
using namespace xplpc::serializer;
using namespace xplpc::util;

class Client
{
public:
    Client() = default;

    template <typename T>
    static void call(const Request &request, const std::function<void(const std::optional<T> &)> &callback)
    {
        // check if have any platform proxy configured
        if (!PlatformProxyList::shared()->count())
        {
            spdlog::error("[Client : call] No platform proxy was configured");

            if (callback)
            {
                callback(std::nullopt);
            }

            return;
        }

        // generate unique id for callback
        const auto key = UniqueID::generate();

        // add the callback to the list
        // clang-format off
        CallbackList::shared()->add(key, [callback](const std::string &response) {
            std::optional<T> decodedData;

            try
            {
                decodedData = Serializer::decodeFunctionReturnValue<T>(response);
            }
            catch (const std::exception &e)
            {
                spdlog::error("[Client : call] Error when decode data: {}", e.what());
                decodedData = std::nullopt;
            }

            if (callback)
            {
                callback(decodedData);
            }
        });
        // clang-format on

        // find the mapped function in proxy list
        auto functionName = request.functionName();
        auto found = false;

        for (auto const &proxy : PlatformProxyList::shared()->list)
        {
            if (proxy->hasMapping(functionName))
            {
                // call the platform proxy mapped function
                proxy->callProxy(key, request.data());
                found = true;
                break;
            }
        }

        if (!found)
        {
            spdlog::error("[Client : call] Function not found: {0}", functionName);
            callback(std::nullopt);
        }
    }

    static void call(const std::string &requestData, const std::function<void(const std::string &)> callback)
    {
        // check if have any platform proxy configured
        if (!PlatformProxyList::shared()->count())
        {
            spdlog::error("[Client : call] No platform proxy was configured");

            if (callback)
            {
                callback("");
            }

            return;
        }

        // generate unique id for callback
        const auto key = UniqueID::generate();

        // clang-format off
        // add the callback to the list
        CallbackList::shared()->add(key, [callback](const std::string& response) {
            if (callback)
            {
                callback(response);
            }
        });
        // clang-format on

        // find the mapped function in proxy list
        auto functionName = Serializer::decodeFunctionName(requestData);
        auto found = false;

        for (auto const &proxy : PlatformProxyList::shared()->list)
        {
            if (proxy->hasMapping(functionName))
            {
                // call the platform proxy mapped function
                proxy->callProxy(key, requestData);
                found = true;
                break;
            }
        }

        if (!found)
        {
            spdlog::error("[Client : call] Function not found: {0}", functionName);
            callback("");
        }
    }

#if defined(__EMSCRIPTEN__)
    static void call(const std::string &requestData, emscripten::val callback)
    {
        // check if have any platform proxy configured
        if (!PlatformProxyList::shared()->count())
        {
            spdlog::error("[Client : call] No platform proxy was configured");
            callback(std::string{});
            return;
        }

        // generate unique id for callback
        const auto key = UniqueID::generate();

        // clang-format off
        // add the callback to the list
        CallbackList::shared()->add(key, [callback](const std::string& response) {
            callback(response);
        });
        // clang-format on

        // find the mapped function in proxy list
        auto functionName = Serializer::decodeFunctionName(requestData);
        auto found = false;

        for (auto const &proxy : PlatformProxyList::shared()->list)
        {
            if (proxy->hasMapping(functionName))
            {
                // call the platform proxy mapped function
                proxy->callProxy(key, requestData);
                found = true;
                break;
            }
        }

        if (!found)
        {
            spdlog::error("[Client : call] Function not found: {0}", functionName);
            callback(std::string{});
        }
    }
#endif
};

} // namespace client
} // namespace xplpc
