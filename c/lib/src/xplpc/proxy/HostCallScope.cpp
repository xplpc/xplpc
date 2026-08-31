#include "xplpc/proxy/HostCallScope.hpp"

namespace xplpc
{
namespace proxy
{

thread_local int HostCallScope::depth = 0;

} // namespace proxy
} // namespace xplpc
