#pragma once

#include <exception>
#include <optional>
#include <string>

#include "xplpc/data/CallbackList.hpp"
#include "xplpc/message/Request.hpp"
#include "xplpc/proxy/PlatformProxy.hpp"
#include "xplpc/serializer/Serializer.hpp"
#include "xplpc/util/UniqueID.hpp"

#include "spdlog/spdlog.h"

namespace xplpc
{
namespace client
{

using namespace xplpc::data;
using namespace xplpc::message;
using namespace xplpc::proxy;
using namespace xplpc::serializer;
using namespace xplpc::util;

class RemoteClient
{
public:
    RemoteClient() = default;

    template <typename T>
    static void call(const Request &request, const std::function<void(const std::optional<T> &)> &callback)
    {
        if (!PlatformProxy::hasProxy())
        {
            spdlog::error("[RemoteClient : call] Platform proxy was not configured");

            if (callback)
            {
                callback(std::nullopt);
            }

            return;
        }

        const auto key = UniqueID::generate();

        // clang-format off
        CallbackList::shared()->add(key, [callback](const std::string& response) {
            std::optional<T> decodedData;

            try
            {
                decodedData = Serializer::decodeFunctionReturnValue<T>(response);
            }
            catch (const std::exception &e)
            {
                spdlog::error("[RemoteClient : call] Error when decode data: {}", e.what());
                decodedData = std::nullopt;
            }

            if (callback)
            {
                callback(decodedData);
            }
        });
        // clang-format on

        PlatformProxy::shared()->callProxy(key, request.data());
    }
};

} // namespace client
} // namespace xplpc
