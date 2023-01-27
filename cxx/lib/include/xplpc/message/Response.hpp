#pragma once

#include <any>
#include <functional>

namespace xplpc
{
namespace message
{

using Response = std::function<void(const std::any &value)>;

} // namespace message
} // namespace xplpc
