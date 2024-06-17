#include "xplpc/data/MappingList.hpp"

namespace xplpc
{
namespace data
{

std::shared_ptr<MappingList> MappingList::instance = nullptr;
std::once_flag MappingList::initInstanceFlag;

void MappingList::add(const std::string &name, const MappingItem &item)
{
    list[name] = item;
}

std::optional<MappingItem> MappingList::find(const std::string &name)
{
    if (list.find(name) == list.end())
    {
        return std::nullopt;
    }

    return list[name];
}

size_t MappingList::count() const noexcept
{
    return list.size();
}

void MappingList::clear() noexcept
{
    list.clear();
    instance = nullptr;
}

std::shared_ptr<MappingList> MappingList::shared()
{
    std::call_once(initInstanceFlag, []()
                   { instance = std::shared_ptr<MappingList>(new MappingList()); });

    return instance;
}

} // namespace data
} // namespace xplpc
