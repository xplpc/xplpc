#include "xplpc/data/MappingList.hpp"

namespace xplpc
{
namespace data
{

std::shared_ptr<MappingList> MappingList::instance = nullptr;

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
    if (instance == nullptr)
    {
        instance = std::make_shared<MappingList>();
    }

    return instance;
}

} // namespace data
} // namespace xplpc
