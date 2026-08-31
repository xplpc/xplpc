#pragma once

#include <exception>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "xplpc/client/CallAwaitable.hpp"
#include "xplpc/client/SyncAnswer.hpp"
#include "xplpc/core/XPLPC.hpp"
#include "xplpc/data/CallbackList.hpp"
#include "xplpc/data/PlatformProxyList.hpp"
#include "xplpc/message/Request.hpp"
#include "xplpc/serializer/Serializer.hpp"
#include "xplpc/util/Log.hpp"
#include "xplpc/util/UniqueID.hpp"

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
    template <typename T>
    static void call(const Request &request, const std::function<void(const std::optional<T> &)> &callback)
    {
        // clang-format off
        const auto key = dispatch(request.functionName(), request.data(), [callback](const std::string &response) {
            std::optional<T> decodedData;

            try
            {
                decodedData = Serializer::decodeFunctionReturnValue<T>(response);
            }
            catch (const std::exception &e)
            {
                util::Log::e("[Client : call] Error when decode data");
                util::Log::d("[Client : call] Error when decode data: {}", e.what());
            }

            if (callback)
            {
                callback(decodedData);
            }
        });
        // clang-format on

        if (key.empty() && callback)
        {
            callback(std::nullopt);
        }
    }

    static void call(const std::string &requestData, const std::function<void(const std::string &)> &callback)
    {
        // clang-format off
        const auto key = dispatch(Serializer::decodeFunctionName(requestData), requestData, [callback](const std::string &response) {
            if (callback)
            {
                callback(response);
            }
        });
        // clang-format on

        if (key.empty() && callback)
        {
            callback("");
        }
    }

    template <typename T>
    static std::optional<T> callSync(const Request &request)
    {
        const auto response = answerSynchronously(request.functionName(), request.data());

        if (!response.has_value())
        {
            return std::nullopt;
        }

        try
        {
            return Serializer::decodeFunctionReturnValue<T>(response.value());
        }
        catch (const std::exception &e)
        {
            util::Log::e("[Client : callSync] Error when decode data");
            util::Log::d("[Client : callSync] Error when decode data: {}", e.what());
        }

        return std::nullopt;
    }

    static std::string callSync(const std::string &requestData)
    {
        return answerSynchronously(Serializer::decodeFunctionName(requestData), requestData).value_or("");
    }

    template <typename T>
    static CallAwaitable<std::optional<T>> callAsync(const Request &request)
    {
        // clang-format off
        return CallAwaitable<std::optional<T>>([request](const std::function<void(std::optional<T>)> &answer) {
            call<T>(request, [answer](const std::optional<T> &result) {
                answer(result);
            });
        });
        // clang-format on
    }

    static CallAwaitable<std::string> callAsync(const std::string &requestData)
    {
        // clang-format off
        return CallAwaitable<std::string>([requestData](const std::function<void(std::string)> &answer) {
            call(requestData, [answer](const std::string &response) {
                answer(response);
            });
        });
        // clang-format on
    }

#if defined(__EMSCRIPTEN__)
    static void call(const std::string &requestData, emscripten::val callback)
    {
        // clang-format off
        const auto key = dispatch(Serializer::decodeFunctionName(requestData), requestData, [callback](const std::string &response) {
            callback(response);
        });
        // clang-format on

        if (key.empty())
        {
            callback(std::string{});
        }
    }
#endif

private:
    static std::optional<std::string> answerSynchronously(const std::string &functionName, const std::string &requestData)
    {
        // A mapping is free to answer from a thread of its own after this function has returned, so what it writes into outlives the frame and is guarded.
        const auto answer = std::make_shared<SyncAnswer>();

        // clang-format off
        const auto key = dispatch(functionName, requestData, [answer](const std::string &response) {
            answer->set(response);
        });
        // clang-format on

        if (key.empty())
        {
            return std::nullopt;
        }

        // Taking the key back is what decides the two cases, since a mapping that answered inline has already taken it and one that deferred never will.
        CallbackList::shared()->remove(key);

        auto response = answer->get();

        if (!response.has_value())
        {
            util::Log::e("[Client : callSync] The function did not answer synchronously");
        }

        return response;
    }

    static std::string dispatch(const std::string &functionName, const std::string &requestData, Callback callback)
    {
        // The callback is registered and the request goes to the first proxy owning the function, and the registration is dropped when nothing handles it.

        // Routing happens here, so a library that cannot serve a call has to refuse before a mapping runs rather than after it has answered.
        if (!core::XPLPC::isInitialized())
        {
            util::Log::e("[Client : dispatch] The library is not initialized");
            return "";
        }

        if (!PlatformProxyList::shared()->count())
        {
            util::Log::e("[Client : dispatch] No platform proxy was configured");
            return "";
        }

        if (functionName.empty())
        {
            // The decoder is the one that knows whether the document was unreadable or merely carried no name, so it reports and this answers.
            return "";
        }

        const auto key = UniqueID::generate();
        CallbackList::shared()->add(key, std::move(callback));

        // clang-format off
        const auto dispatched = PlatformProxyList::shared()->forEach([&](const std::shared_ptr<PlatformProxy> &proxy) {
            if (!proxy->hasMapping(functionName))
            {
                return false;
            }

            proxy->callProxy(key, requestData);
            return true;
        });
        // clang-format on

        if (!dispatched)
        {
            util::Log::e("[Client : dispatch] Function not found: {}", functionName);
            CallbackList::shared()->remove(key);
            return "";
        }

        return key;
    }
};

} // namespace client
} // namespace xplpc
