#pragma once

#include "xplpc/util/Log.hpp"

#include "spdlog/sinks/ringbuffer_sink.h"

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

// The library publishes its logger under its own name, which is what lets a test read what it reported.
// Only failures are recorded, since a debug line is the reason behind one rather than a failure of its own.
class LogRecorder
{
public:
    LogRecorder()
        : logger(xplpc::util::Log::logger())
        , sink(std::make_shared<spdlog::sinks::ringbuffer_sink_mt>(64))
    {
        sink->set_level(spdlog::level::err);
        logger->sinks().push_back(sink);
    }

    ~LogRecorder()
    {
        auto &sinks = logger->sinks();
        sinks.erase(std::remove(sinks.begin(), sinks.end(), sink), sinks.end());
    }

    LogRecorder(const LogRecorder &) = delete;
    LogRecorder &operator=(const LogRecorder &) = delete;

    std::vector<std::string> errors() const { return sink->last_formatted(); }

    bool reportedNothing() const { return sink->last_formatted().empty(); }

private:
    std::shared_ptr<spdlog::logger> logger;
    std::shared_ptr<spdlog::sinks::ringbuffer_sink_mt> sink;
};
