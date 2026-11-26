#pragma once

#include "fixtures/FakeLayer.hpp"
#include "xplpc/core/Handle.hpp"

#include <deque>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

// Keeps every call a layer received instead of answering it, so a test answers each one when it chooses and from whichever thread it likes.
class HeldCalls
{
public:
    FakeLayer::Behaviour behaviour() const
    {
        // clang-format off
        return [state = state](FakeLayer &, xplpc::core::Handle call, const std::string &, const std::string &) {
            std::lock_guard<std::mutex> lock(state->mutex);
            state->calls.push_back(call);
        };
        // clang-format on
    }

    std::optional<xplpc::core::Handle> next() const
    {
        std::lock_guard<std::mutex> lock(state->mutex);

        if (state->calls.empty())
        {
            return std::nullopt;
        }

        const auto call = state->calls.front();
        state->calls.pop_front();

        return call;
    }

    std::vector<xplpc::core::Handle> all() const
    {
        std::lock_guard<std::mutex> lock(state->mutex);
        return {state->calls.begin(), state->calls.end()};
    }

private:
    struct State
    {
        std::mutex mutex;
        std::deque<xplpc::core::Handle> calls;
    };

    std::shared_ptr<State> state = std::make_shared<State>();
};
