#pragma once

#include "xplpc/client/Client.hpp"
#include "xplpc/message/Request.hpp"

#include "gtest/gtest.h"

#include <exception>
#include <functional>
#include <memory>
#include <optional>
#include <string>

// A callback that never runs would leave every check unexecuted and the test green, so it is verified here.
class VerifiedCall
{
public:
    template <typename T>
    static void run(const xplpc::message::Request &request, const std::function<void(const std::optional<T> &)> &block)
    {
        auto answered = std::make_shared<bool>(false);

        // clang-format off
        xplpc::client::Client::call<T>(request, [answered, block](const std::optional<T> &response) {
            *answered = true;
            check([&] { block(response); });
        });
        // clang-format on

        EXPECT_TRUE(*answered) << "the callback was never invoked";
    }

    static void run(const std::string &requestData, const std::function<void(const std::string &)> &block)
    {
        auto answered = std::make_shared<bool>(false);

        // clang-format off
        xplpc::client::Client::call(requestData, [answered, block](const std::string &response) {
            *answered = true;
            check([&] { block(response); });
        });
        // clang-format on

        EXPECT_TRUE(*answered) << "the callback was never invoked";
    }

private:
    // The library reports a callback that raises and keeps going, so an assertion the raise skipped would otherwise pass.
    template <typename Block>
    static void check(Block &&body)
    {
        try
        {
            body();
        }
        catch (const std::exception &e)
        {
            ADD_FAILURE() << "the callback raised before its checks finished: " << e.what();
        }
        catch (...)
        {
            ADD_FAILURE() << "the callback raised before its checks finished";
        }
    }
};
