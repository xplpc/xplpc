#pragma once

#include "spdlog/spdlog.h"

#include <any>
#include <exception>
#include <map>
#include <optional>
#include <string>

namespace xplpc
{
namespace message
{

class Message : public std::map<std::string, std::any>
{
public:
    template <typename T>
    const std::optional<T> get(const std::string &name) const
    {
        try
        {
            if (count(name))
            {
                return std::any_cast<T>(at(name));
            }
            else
            {
                spdlog::debug("[Message : get] Key not exists: {}", name);
            }
        }
        catch (const std::exception &e)
        {
            spdlog::error("[Message : get] Error when cast value: {}", e.what());
        }

        return std::nullopt;
    }
};

} // namespace message
} // namespace xplpc
