#include "xplpc/proxy/PlatformProxy.hpp"
#include "xplpc/core/XPLPC.hpp"
#include "xplpc/data/CallbackList.hpp"

namespace xplpc
{
namespace proxy
{

using namespace xplpc::core;
using namespace xplpc::data;

std::shared_ptr<PlatformProxy> PlatformProxy::proxy = nullptr;

void PlatformProxy::create(std::shared_ptr<PlatformProxy> proxy)
{
    PlatformProxy::proxy = proxy;
}

void PlatformProxy::createFromPtr(PlatformProxy *proxy)
{
    PlatformProxy::proxy = std::shared_ptr<PlatformProxy>(proxy);
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

void PlatformProxy::callProxy(const std::string &key, const std::string &data)
{
#ifdef XPLPC_SERIALIZER_JSON
    callProxyCallback(key, std::string{R"({"r":null})"});
#else
    callProxyAsyncCallback(key, std::string{});
#endif
}

void PlatformProxy::callProxyCallback(const std::string &key, const std::string &data)
{
    CallbackList::shared()->execute(key, data);
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
cxx/custom/src/xplpc/custom/PlatformProxy.cpp
*/

} // namespace proxy
} // namespace xplpc
