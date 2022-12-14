#pragma once

#include "spdlog/spdlog.h"

#include <any>
#include <exception>
#include <map>
#include <optional>
#include <string>
#include <utility>

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
                spdlog::error("[message : get] Key not exists: {}", name);
            }
        }
        catch (std::exception &e)
        {
            spdlog::error("[message : get] Error when try cast value to required type: {}", e.what());
        }

        return std::nullopt;
    }
};

} // namespace message
} // namespace xplpc
