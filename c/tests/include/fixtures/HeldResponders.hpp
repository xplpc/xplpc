#pragma once

#include "xplpc/message/Response.hpp"

#include <cstddef>
#include <mutex>
#include <string>
#include <vector>

// Keeps the responder of every call a mapping received, so a test decides when and from which thread each one is answered.
class HeldResponders
{
public:
    void add(const xplpc::message::Response &responder)
    {
        std::lock_guard<std::mutex> lock(mutex);
        responders.push_back(responder);
    }

    void answerAll(const std::string &value)
    {
        answerEvery(0, 1, value);
    }

    // Answers every responder from the first one given, stepping by the stride, so several threads can split the held calls between them.
    void answerEvery(size_t first, size_t stride, const std::string &value)
    {
        std::vector<xplpc::message::Response> taken;

        {
            std::lock_guard<std::mutex> lock(mutex);

            for (size_t n = first; n < responders.size(); n += stride)
            {
                taken.push_back(responders[n]);
            }
        }

        for (const auto &responder : taken)
        {
            responder(value);
        }
    }

private:
    std::mutex mutex;
    std::vector<xplpc::message::Response> responders;
};
