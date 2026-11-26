#pragma once

#include "xplpc/core/CallState.hpp"

#include <cstddef>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

// Records every answer a completion received, so a test can tell an answer that never came from one that came twice.
class CompletionRecorder
{
public:
    xplpc::core::Completion completion() const
    {
        // clang-format off
        return [state = state](const std::string &response) {
            std::lock_guard<std::mutex> lock(state->mutex);
            state->responses.push_back(response);
        };
        // clang-format on
    }

    std::vector<std::string> responses() const
    {
        std::lock_guard<std::mutex> lock(state->mutex);
        return state->responses;
    }

    size_t count() const
    {
        return responses().size();
    }

private:
    struct State
    {
        std::mutex mutex;
        std::vector<std::string> responses;
    };

    std::shared_ptr<State> state = std::make_shared<State>();
};
