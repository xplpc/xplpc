#include "xplpc/core/XPLPC.hpp"
#include "spdlog/spdlog.h"

#if defined(__ANDROID__)
#include "spdlog/sinks/android_sink.h"
#endif

namespace xplpc
{
namespace core
{

bool XPLPC::initialized = false;

void XPLPC::initialize()
{
#ifdef XPLPC_ENABLE_LOG
    spdlog::set_level(spdlog::level::trace);
#else
    spdlog::set_level(spdlog::level::err);
#endif

#if defined(__ANDROID__)
    auto logger = spdlog::android_logger_mt("android", "XPLPC");
    spdlog::set_default_logger(logger);
#endif

    initialized = true;
}

bool XPLPC::isInitialized()
{
    return initialized;
}

} // namespace core
} // namespace xplpc
