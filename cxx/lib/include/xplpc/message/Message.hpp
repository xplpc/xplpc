#pragma once

#include "spdlog/spdlog.h"

#include <any>
#include <exception>
#include <optional>
#include <string>
#include <unordered_map>

namespace xplpc
{
namespace message
{

class Message : public std::unordered_map<std::string, std::any>
{
public:
    template <typename T>
    const std::optional<T> get(const std::string &name) const
    {
        try
        {
            auto it = find(name);
            if (it != end())
            {
                return std::any_cast<T>(it->second);
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
