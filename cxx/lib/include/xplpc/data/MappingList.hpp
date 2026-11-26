#pragma once

#include "xplpc/map/MappingItem.hpp"
#include "xplpc/util/TSMap.hpp"

#include <cstddef>
#include <memory>
#include <mutex>
#include <optional>
#include <string>

namespace xplpc
{
namespace data
{

using namespace xplpc::map;
using namespace xplpc::util;

class MappingList
{
public:
    void add(const std::string &name, const MappingItem &item);
    std::optional<MappingItem> find(const std::string &name) const;
    bool has(const std::string &name) const;
    size_t count() const;
    void clear();
    static std::shared_ptr<MappingList> shared();

private:
    TSMap<std::string, MappingItem> list;

    // Adding and clearing each change the list and the names the router holds, so one finishes both before the other starts.
    std::mutex registration;

    MappingList() = default;
    MappingList(const MappingList &) = delete;
    MappingList &operator=(const MappingList &) = delete;
};

} // namespace data
} // namespace xplpc
