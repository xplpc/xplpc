#include "xplpc/util/UniqueID.hpp"

namespace xplpc
{
namespace util
{

std::atomic<std::uint64_t> UniqueID::uid{0};

std::string UniqueID::generate()
{
    return "CXX-" + std::to_string(uid.fetch_add(1, std::memory_order_relaxed) + 1);
}

} // namespace util
} // namespace xplpc
