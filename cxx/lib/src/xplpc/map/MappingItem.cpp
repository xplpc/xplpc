#include "xplpc/map/MappingItem.hpp"

#include <utility>

namespace xplpc
{
namespace map
{

MappingItem::MappingItem(Executor executor)
    : executor(std::move(executor))
{
}

const Executor &MappingItem::getExecutor() const
{
    return executor;
}

} // namespace map
} // namespace xplpc
