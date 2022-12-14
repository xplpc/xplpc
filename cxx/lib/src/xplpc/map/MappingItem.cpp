#include "xplpc/map/MappingItem.hpp"

namespace xplpc
{
namespace map
{

MappingItem::MappingItem(const std::string &name, const std::function<Response(const Message &)> &target, const std::function<const std::string(const std::string &)> &executor)
{
    this->name = name;
    this->target = target;
    this->executor = executor;
}

std::string &MappingItem::getName()
{
    return name;
}

std::function<Response(const Message &)> &MappingItem::getTarget()
{
    return target;
}

const std::function<const std::string(const std::string &)> &MappingItem::getExecutor()
{
    return executor;
}

} // namespace map
} // namespace xplpc
