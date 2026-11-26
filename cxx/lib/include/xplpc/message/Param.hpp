#pragma once

#include <optional>
#include <string>

namespace xplpc
{
namespace message
{

template <typename T>
class Param
{
public:
    Param(const std::string &name, T value)
        : n(name)
        , v(value)
    {
    }

    Param(const std::string &name, std::nullopt_t)
        : n(name)
        , v(std::nullopt)
    {
    }

    std::string n;
    std::optional<T> v;
};

} // namespace message
} // namespace xplpc
