#pragma once

#include "xplpc/map/MappingItem.hpp"

#include <cstddef>
#include <map>
#include <memory>
#include <mutex>
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
    void add(const std::string &name, const MappingItem &item);
    std::optional<MappingItem> find(const std::string &name);
    size_t count() const noexcept;
    void clear() noexcept;
    static std::shared_ptr<MappingList> shared();

private:
    static std::shared_ptr<MappingList> instance;
    static std::once_flag initInstanceFlag;
    std::map<std::string, MappingItem> list;

    MappingList() = default;
    MappingList(const MappingList &) = delete;
    MappingList &operator=(const MappingList &) = delete;
};

} // namespace data
} // namespace xplpc
