#include "xplpc/custom/Mapping.hpp"

namespace xplpc
{
namespace proxy
{

void PlatformProxy::initializePlatform()
{
    xplpc::custom::Mapping::initialize();
}

} // namespace proxy
} // namespace xplpc
