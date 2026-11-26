#pragma once

#include <functional>
#include <optional>
#include <string>
#include <utility>

#include "xplpc/client/CallAwaitable.hpp"
#include "xplpc/core/CallResult.hpp"
#include "xplpc/core/CallState.hpp"
#include "xplpc/core/Router.hpp"
#include "xplpc/message/Request.hpp"
#include "xplpc/serializer/Serializer.hpp"
#include "xplpc/util/Log.hpp"

namespace xplpc
{
namespace client
{

using namespace xplpc::message;
using namespace xplpc::serializer;

class Client
{
public:
    template <typename T>
    static void call(const Request &request, const std::function<void(const std::optional<T> &)> &callback)
    {
        // clang-format off
        const core::Completion completion = [callback](const std::string &response) {
            const auto value = Serializer::decodeFunctionReturnValue<T>(response);

            if (callback)
            {
                callback(value);
            }
        };
        // clang-format on

        deliver(route(request, completion), completion);
    }

    static void call(const std::string &requestData, const std::function<void(const std::string &)> &callback)
    {
        // clang-format off
        const core::Completion completion = [callback](const std::string &response) {
            if (callback)
            {
                callback(response);
            }
        };
        // clang-format on

        deliver(core::Router::shared().call(core::Router::shared().native(), requestData, completion), completion);
    }

    template <typename T>
    static std::optional<T> callSync(const Request &request)
    {
        const auto response = answerSynchronously(route(request, nullptr));

        if (!response.has_value())
        {
            return std::nullopt;
        }

        return Serializer::decodeFunctionReturnValue<T>(response.value());
    }

    static std::string callSync(const std::string &requestData)
    {
        return answerSynchronously(core::Router::shared().call(core::Router::shared().native(), requestData, nullptr)).value_or("");
    }

    template <typename T>
    static CallAwaitable<std::optional<T>> callAsync(const Request &request)
    {
        // clang-format off
        return CallAwaitable<std::optional<T>>([request](const CallAwaitable<std::optional<T>>::Answer &answer) {
            const auto result = route(request, [answer](const std::string &response) {
                answer(Serializer::decodeFunctionReturnValue<T>(response));
            });

            if (result.isAnswered())
            {
                answer(Serializer::decodeFunctionReturnValue<T>(result.value()));
            }

            return result.handle();
        });
        // clang-format on
    }

    static CallAwaitable<std::string> callAsync(const std::string &requestData)
    {
        // clang-format off
        return CallAwaitable<std::string>([requestData](const CallAwaitable<std::string>::Answer &answer) {
            const auto result = core::Router::shared().call(core::Router::shared().native(), requestData, [answer](const std::string &response) {
                answer(response);
            });

            if (result.isAnswered())
            {
                answer(result.value());
            }

            return result.handle();
        });
        // clang-format on
    }

private:
    static core::CallResult route(const Request &request, core::Completion completion)
    {
        auto &router = core::Router::shared();
        return router.call(router.native(), request.functionName(), request.data(), std::move(completion));
    }

    static void deliver(const core::CallResult &result, const core::Completion &completion)
    {
        // An answer the mapping gave before the call returned is handed over here, through the same guard a late answer goes through.

        if (result.isAnswered())
        {
            core::CallState::run(completion, result.value());
        }
    }

    static std::optional<std::string> answerSynchronously(const core::CallResult &result)
    {
        // A mapping that had not answered when the call returned is cancelled, so its later answer resolves nothing and leaves nothing behind.

        if (result.isAnswered())
        {
            return result.value();
        }

        core::Router::shared().cancel(core::Router::shared().native(), result.handle());
        util::Log::e("[Client : callSync] The function did not answer synchronously");

        return std::nullopt;
    }
};

} // namespace client
} // namespace xplpc
