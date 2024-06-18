#include "xplpc/util/UniqueID.hpp"

namespace xplpc
{
namespace util
{

std::atomic<std::uint64_t> UniqueID::uid{0};

std::string UniqueID::generate()
{
    uint64_t id = uid.fetch_add(1, std::memory_order_relaxed);
    return "CXX-" + std::to_string(id);
}

} // namespace util
} // namespace xplpc
