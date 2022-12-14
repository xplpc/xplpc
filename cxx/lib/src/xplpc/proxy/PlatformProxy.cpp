#include "xplpc/proxy/PlatformProxy.hpp"
#include "xplpc/core/XPLPC.hpp"
#include <utility>

namespace xplpc
{
namespace proxy
{

using namespace xplpc::core;

std::shared_ptr<PlatformProxy> PlatformProxy::proxy = nullptr;

void PlatformProxy::create(std::shared_ptr<PlatformProxy> proxy)
{
    PlatformProxy::proxy = proxy;
}

void PlatformProxy::createDefault()
{
    PlatformProxy::proxy = std::make_shared<PlatformProxy>();
}

std::shared_ptr<PlatformProxy> PlatformProxy::shared()
{
    return proxy;
}

bool PlatformProxy::hasProxy()
{
    return (proxy != nullptr);
}

void PlatformProxy::initialize()
{
    XPLPC::initialize();
    initializePlatform();
}

void PlatformProxy::finalize()
{
    // ignore
}

std::string PlatformProxy::call(const std::string &data)
{
    return "";
}

/*
- ALL PLATFORMS NEED IMPLEMENT "CUSTOM INITIALIZE METHOD".
- IT CAN BE EMPTY, BUT NEED EXISTS.
- ALL MAPPINGS IN C++ SIDE CAN BE INSIDE THIS METHOD.

> EXAMPLE:

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

> SEE:

/cxx/custom/src/xplpc/custom/PlatformProxy.cpp
*/

} // namespace proxy
} // namespace xplpc
