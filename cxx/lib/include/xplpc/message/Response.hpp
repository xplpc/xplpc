#pragma once

#include <any>
#include <functional>
#include <string>

namespace xplpc
{
namespace message
{

using Response = std::function<void(const std::any &value)>;

} // namespace message
} // namespace xplpc
