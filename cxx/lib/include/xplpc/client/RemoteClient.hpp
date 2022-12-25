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
            return Serializer::decodeFunctionReturnValue<T>(PlatformProxy::shared()->call(request.data()));
        }
        catch (std::exception &e)
        {
            spdlog::error("[RemoteClient : call] Error when try to convert return value");
        }

        return std::nullopt;
    }

    template <typename T>
    static std::future<std::optional<T>> callAsync(const Request &request)
    {
        return std::async([&]()
                          { return call<T>(request); });
    }
};

} // namespace client
} // namespace xplpc
