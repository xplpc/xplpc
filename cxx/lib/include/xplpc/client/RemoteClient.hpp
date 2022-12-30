#pragma once

#include <exception>
#include <future>
#include <optional>
#include <utility>

#include "xplpc/message/Request.hpp"
#include "xplpc/proxy/PlatformProxy.hpp"
#include "xplpc/serializer/Serializer.hpp"

#include "spdlog/spdlog.h"

namespace xplpc
{
namespace client
{

using namespace xplpc::serializer;
using namespace xplpc::data;
using namespace xplpc::proxy;

class RemoteClient
{
public:
    RemoteClient() = default;

    template <typename T>
    static std::optional<T> call(const Request &request)
    {
        if (!PlatformProxy::hasProxy())
        {
            spdlog::error("[RemoteClient : call] Platform proxy was not configured");
            return std::nullopt;
        }

        try
        {
            return Serializer::decodeFunctionReturnValue<T>(PlatformProxy::shared()->callProxy(request.data()));
        }
        catch (std::exception &e)
        {
            spdlog::error("[RemoteClient : call] Error when try to convert return value");
        }

        return std::nullopt;
    }

    template <typename T>
    static void callAsync(const Request &request, std::function<void(const std::optional<T> &)> callback)
    {
        if (!PlatformProxy::hasProxy())
        {
            spdlog::error("[RemoteClient : callAsync] Platform proxy was not configured");
            callback(std::nullopt);
        }

        try
        {
            spdlog::info("[RemoteClient : callAsync] 1");

            // TODO: XPLPC - WHAT I NEED USE FOR WASM CALLBACK?
            PlatformProxy::shared()->callProxyAsync(request.data(), [&](const std::string &data)
                                                    {
                spdlog::info("[RemoteClient : callAsync] 2 with data: {}", data);
                callback(Serializer::decodeFunctionReturnValue<T>(data)); });
        }
        catch (std::exception &e)
        {
            spdlog::error("[RemoteClient : callAsync] Error when try to convert return value");
            callback(std::nullopt);
        }
    }
};

} // namespace client
} // namespace xplpc
