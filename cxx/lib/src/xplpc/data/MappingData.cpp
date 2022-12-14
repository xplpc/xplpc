#include "xplpc/data/MappingData.hpp"

namespace xplpc
{
namespace data
{

std::shared_ptr<MappingData> MappingData::instance = nullptr;

void MappingData::add(const MappingItem &item)
{
    shared()->getList().push_back(item);
}

std::vector<MappingItem> &MappingData::getList()
{
    return list;
}

std::optional<MappingItem> MappingData::find(const std::string &name)
{
    // clang-format off
    auto mappingItem = std::find_if(MappingData::shared()->getList().begin(), MappingData::shared()->getList().end(), [&name](MappingItem &item){ return item.getName() == name; });
    // clang-format on

    if (mappingItem != MappingData::shared()->getList().end())
    {
        return *mappingItem;
    }

    return std::nullopt;
}

std::shared_ptr<MappingData> MappingData::shared()
{
    if (instance == nullptr)
    {
        instance = std::make_shared<MappingData>();
    }

    return instance;
}

} // namespace data
} // namespace xplpc
