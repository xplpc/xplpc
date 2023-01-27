#pragma once

#include <optional>
#include <string>
#include <utility>

namespace xplpc
{
namespace message
{

template <typename T>
class Param
{
public:
    Param(const std::string &name, const T &value)
    {
        this->n = name;
        this->v = value;
    }

    std::string n;
    std::optional<T> v;
};

} // namespace message
} // namespace xplpc
