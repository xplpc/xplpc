#include "xplpc/data/MappingList.hpp"
#include "xplpc/core/Router.hpp"

namespace xplpc
{
namespace data
{

void MappingList::add(const std::string &name, const MappingItem &item)
{
    std::lock_guard<std::mutex> lock(registration);

    // The mapping is recorded before the name is declared, so a call routed the moment the router hears of it already finds it.
    list.set(name, item);
    core::Router::shared().declare(core::Router::shared().native(), name);
}

std::optional<MappingItem> MappingList::find(const std::string &name) const
{
    return list.get(name);
}

bool MappingList::has(const std::string &name) const
{
    return list.contains(name);
}

size_t MappingList::count() const
{
    return list.count();
}

void MappingList::clear()
{
    std::lock_guard<std::mutex> lock(registration);

    for (const auto &entry : list.takeAll())
    {
        core::Router::shared().retract(core::Router::shared().native(), entry.first);
    }
}

std::shared_ptr<MappingList> MappingList::shared()
{
    // The list is never destroyed, since a mapping answering from a thread of its own can still reach it while the process exits.
    static const auto *instance = new std::shared_ptr<MappingList>(new MappingList());
    return *instance;
}

} // namespace data
} // namespace xplpc
