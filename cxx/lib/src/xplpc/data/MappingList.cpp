#include "xplpc/data/MappingList.hpp"

namespace xplpc
{
namespace data
{

std::shared_ptr<MappingList> MappingList::instance = nullptr;
std::once_flag MappingList::initInstanceFlag;

void MappingList::add(const std::string &name, const MappingItem &item)
{
    list.set(name, item);
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
    list.clear();
}

std::shared_ptr<MappingList> MappingList::shared()
{
    // clang-format off
    std::call_once(initInstanceFlag, []() {
        instance = std::shared_ptr<MappingList>(new MappingList());
    });
    // clang-format on

    return instance;
}

} // namespace data
} // namespace xplpc
