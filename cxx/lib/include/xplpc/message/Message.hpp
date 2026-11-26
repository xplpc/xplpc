#pragma once

#include "xplpc/util/Log.hpp"

#include <any>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>

namespace xplpc
{
namespace message
{

class Message
{
public:
    template <typename T>
    std::optional<T> get(const std::string &name) const
    {
        auto it = values.find(name);

        if (it == values.end())
        {
            return std::nullopt;
        }

        if (const auto *value = std::any_cast<T>(&it->second))
        {
            return *value;
        }

        util::Log::e("[Message : get] Value of \"{}\" has another type", name);

        return std::nullopt;
    }

    void set(const std::string &name, std::any value)
    {
        values[name] = std::move(value);
    }

private:
    std::unordered_map<std::string, std::any> values;
};

} // namespace message
} // namespace xplpc
