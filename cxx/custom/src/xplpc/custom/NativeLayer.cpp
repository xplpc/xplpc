#include "xplpc/custom/Mapping.hpp"

namespace xplpc
{
namespace layer
{

void NativeLayer::initializePlatform()
{
    xplpc::custom::Mapping::initialize();
}

} // namespace layer
} // namespace xplpc
