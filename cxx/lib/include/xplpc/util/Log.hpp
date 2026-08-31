#pragma once

#include "spdlog/spdlog.h"

#include <memory>
#include <string>
#include <utility>

namespace xplpc
{
namespace util
{

class Log
{
public:
    // The logger is published under its own name, so a host can send this library's lines wherever it wants without touching its own.
    static const std::shared_ptr<spdlog::logger> &logger();

    // clang-format off
    template <typename... Args>
    static void d(spdlog::format_string_t<Args...> format, Args &&...args)
    {
        logger()->debug(format, std::forward<Args>(args)...);
    }

    template <typename... Args>
    static void e(spdlog::format_string_t<Args...> format, Args &&...args)
    {
        logger()->error(format, std::forward<Args>(args)...);
    }
    // clang-format on

private:
    static std::shared_ptr<spdlog::logger> create();
    static std::string requestedLevel();
    static void applyLevel(const std::shared_ptr<spdlog::logger> &target);
};

} // namespace util
} // namespace xplpc
