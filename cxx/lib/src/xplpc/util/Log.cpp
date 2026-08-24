#include "xplpc/util/Log.hpp"

#if defined(__ANDROID__)
#include "spdlog/sinks/android_sink.h"
#else
#include "spdlog/sinks/stdout_color_sinks.h"
#endif

#if defined(__ANDROID__)
#include <sys/system_properties.h>
#endif

#include <cstdlib>
#include <string>

namespace xplpc
{
namespace util
{

std::shared_ptr<spdlog::logger> Log::create()
{
#if defined(__ANDROID__)
    auto sink = std::make_shared<spdlog::sinks::android_sink_mt>("XPLPC");
#else
    auto sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
#endif

    auto logger = std::make_shared<spdlog::logger>("XPLPC", sink);

    applyLevel(logger);

    // Publishing the logger is what lets a host redirect this library alone, and a host that already holds the name keeps the logger it made and the level it chose.
    if (!spdlog::get("XPLPC"))
    {
        spdlog::register_logger(logger);
    }

    return logger;
}

const std::shared_ptr<spdlog::logger> &Log::logger()
{
    // The library keeps a logger of its own, so a host that uses spdlog never has its default logger or its level changed by this one.
    // The level is read where the logger is built, so the first line this library writes is already filtered the way the environment asked for.

    static const std::shared_ptr<spdlog::logger> logger = create();

    return logger;
}

std::string Log::requestedLevel()
{
#if defined(__ANDROID__)
    // An android application cannot put a variable in its own environment, so the level is read where that platform keeps a runtime setting.
    char property[PROP_VALUE_MAX] = {};

    if (__system_property_get("debug.xplpc.level", property) > 0)
    {
        return property;
    }
#endif

    if (const auto *value = std::getenv("XPLPC_LOG_LEVEL"))
    {
        return value;
    }

    return {};
}

void Log::applyLevel(const std::shared_ptr<spdlog::logger> &target)
{
    target->set_level(spdlog::level::err);

    const auto value = requestedLevel();

    if (value.empty())
    {
        return;
    }

    const auto level = spdlog::level::from_str(value);

    if (level == spdlog::level::off && value != "off")
    {
        target->error("[Log : applyLevel] The requested level does not name one, so only errors are reported");
        return;
    }

    target->set_level(level);
}

} // namespace util
} // namespace xplpc
