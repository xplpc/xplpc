#include "xplpc/map/MappingItem.hpp"

namespace xplpc
{
namespace map
{

MappingItem::MappingItem(const std::function<void(const Message &, const Response)> &target, const std::function<void(const std::string &, const std::string &)> &executor)
{
    this->target = target;
    this->executor = executor;
}

std::function<void(const Message &, const Response)> &MappingItem::getTarget()
{
    return target;
}

const std::function<void(const std::string &, const std::string &)> &MappingItem::getExecutor()
{
    return executor;
}

} // namespace map
} // namespace xplpc
