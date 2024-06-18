#import "xplpc/proxy/ObjCPlatformProxy.h"
#import "xplpc/objc/support.h"

namespace xplpc
{
namespace proxy
{

std::shared_ptr<ObjCPlatformProxy> ObjCPlatformProxy::instance = nullptr;
std::once_flag ObjCPlatformProxy::initInstanceFlag;

std::shared_ptr<ObjCPlatformProxy> ObjCPlatformProxy::shared()
{
    // clang-format off
    std::call_once(initInstanceFlag, []() {
        instance = std::shared_ptr<ObjCPlatformProxy>(new ObjCPlatformProxy());
    });
    // clang-format on

    return instance;
}

void ObjCPlatformProxy::initialize()
{
    initializePlatform();
}

void ObjCPlatformProxy::initializePlatform()
{
    if (proxyImpl != nullptr)
    {
        [proxyImpl onInitializePlatform];
    }
}

void ObjCPlatformProxy::finalize()
{
    finalizePlatform();
}

void ObjCPlatformProxy::finalizePlatform()
{
    if (proxyImpl != nullptr)
    {
        [proxyImpl onFinalizePlatform];
    }

    proxyImpl = nullptr;
}

void ObjCPlatformProxy::callProxy(const std::string &key, const std::string &data)
{
    if (proxyImpl != nullptr)
    {
        [proxyImpl onNativeProxyCall:ObjCSupport::xplpcCppStringToObjcString(key):ObjCSupport::xplpcCppStringToObjcString(data)];
    }
}

bool ObjCPlatformProxy::hasMapping(const std::string &name)
{
    if (proxyImpl != nullptr)
    {
        return [proxyImpl onHasMapping:ObjCSupport::xplpcCppStringToObjcString(name)];
    }

    return false;
}

void ObjCPlatformProxy::setProxyImpl(ObjCPlatformProxyImpl *proxyImpl)
{
    this->proxyImpl = proxyImpl;
}

} // namespace proxy
} // namespace xplpc
