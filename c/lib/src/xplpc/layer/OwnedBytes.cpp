#include "xplpc/layer/OwnedBytes.hpp"

#include <algorithm>
#include <cstdlib>
#include <cstring>

namespace xplpc
{
namespace layer
{

char *OwnedBytes::copy(const std::string &value)
{
    auto owned = static_cast<char *>(std::malloc(std::max<size_t>(value.size(), 1)));

    if (owned)
    {
        std::memcpy(owned, value.data(), value.size());
    }

    return owned;
}

} // namespace layer
} // namespace xplpc
