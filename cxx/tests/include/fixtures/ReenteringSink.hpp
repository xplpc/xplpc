#pragma once

#include "xplpc/util/Log.hpp"

#include "spdlog/sinks/callback_sink.h"

#include <algorithm>
#include <atomic>
#include <functional>
#include <memory>

// A sink is code of the host's own, which may reach back into the library from the line it is handed, so every failure has to be reported with nothing held.
class ReenteringSink
{
public:
    explicit ReenteringSink(std::function<void()> reentry)
        : logger(xplpc::util::Log::logger())
    {
        // clang-format off
        sink = std::make_shared<spdlog::sinks::callback_sink_mt>([this, reentry](const spdlog::details::log_msg &) {
            reentry();
            ++reports;
        });
        // clang-format on

        sink->set_level(spdlog::level::err);
        logger->sinks().push_back(sink);
    }

    ~ReenteringSink()
    {
        auto &sinks = logger->sinks();
        sinks.erase(std::remove(sinks.begin(), sinks.end(), sink), sinks.end());
    }

    ReenteringSink(const ReenteringSink &) = delete;
    ReenteringSink &operator=(const ReenteringSink &) = delete;

    int count() const { return reports.load(); }

private:
    std::shared_ptr<spdlog::logger> logger;
    std::shared_ptr<spdlog::sinks::callback_sink_mt> sink;
    std::atomic<int> reports{0};
};
