#pragma once

#include "xplpc/message/Message.hpp"
#include "xplpc/message/Response.hpp"

#include <chrono>
#include <string>
#include <thread>

// The answer comes from a thread that outlives the call, which is what a mapping that is genuinely asynchronous does.
class DetachedAnswerMapping
{
public:
    static void target(const xplpc::message::Message & /*m*/, const xplpc::message::Response &r)
    {
        // clang-format off
        std::thread([r] {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            r(std::string("detached"));
        }).detach();
        // clang-format on
    }
};
