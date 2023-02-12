#pragma once

#include "xplpc/map/MappingItem.hpp"

#include <cstddef>
#include <map>
#include <memory>
#include <optional>
#include <string>

namespace xplpc
{
namespace data
{

using namespace xplpc::map;

class MappingList
{
public:
    MappingList() = default;
    void add(const std::string &name, const MappingItem &item);
    std::optional<MappingItem> find(const std::string &name);
    size_t count() const noexcept;
    static std::shared_ptr<MappingList> shared();

private:
    static std::shared_ptr<MappingList> instance;
    std::map<std::string, MappingItem> list;
};

} // namespace data
} // namespace xplpc
