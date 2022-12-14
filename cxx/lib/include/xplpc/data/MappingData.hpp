#pragma once

#include "xplpc/map/MappingItem.hpp"

#include <memory>
#include <optional>
#include <vector>

namespace xplpc
{
namespace data
{

using namespace xplpc::map;

class MappingData
{
public:
    MappingData() = default;
    static void add(const MappingItem &item);
    static std::shared_ptr<MappingData> shared();
    static std::optional<MappingItem> find(const std::string &name);
    std::vector<MappingItem> &getList();

private:
    static std::shared_ptr<MappingData> instance;
    std::vector<MappingItem> list;
};

} // namespace data
} // namespace xplpc
