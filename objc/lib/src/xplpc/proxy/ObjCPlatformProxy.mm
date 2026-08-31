#import "xplpc/proxy/ObjCPlatformProxy.h"
#import "xplpc/core/XPLPC.hpp"
#import "xplpc/data/CallbackList.hpp"
#import "xplpc/data/PlatformProxyList.hpp"
#import "xplpc/objc/support.h"
#import "xplpc/proxy/NativePlatformProxy.hpp"
#import "xplpc/util/Log.hpp"

namespace xplpc
{
namespace proxy
{

using namespace xplpc::data;

std::shared_ptr<ObjCPlatformProxy> ObjCPlatformProxy::instance = nullptr;
std::once_flag ObjCPlatformProxy::initInstanceFlag;
std::once_flag ObjCPlatformProxy::proxyRegistrationFlag;

std::shared_ptr<ObjCPlatformProxy> ObjCPlatformProxy::shared()
{
    // clang-format off
    std::call_once(initInstanceFlag, []() {
        instance = std::shared_ptr<ObjCPlatformProxy>(new ObjCPlatformProxy());
    });
    // clang-format on

    return instance;
}

void ObjCPlatformProxy::registerProxies()
{
    // The framework constructor may run more than once per process, so the list is populated only on the first pass.

    // clang-format off
    std::call_once(proxyRegistrationFlag, []() {
        auto nativePlatformProxy = std::make_shared<NativePlatformProxy>();
        nativePlatformProxy->initialize();

        PlatformProxyList::shared()->prepend(nativePlatformProxy);
        PlatformProxyList::shared()->prepend(shared());

        // The flag is set once both proxies are registered, so nothing reports itself as ready while a call would find no host.
        core::XPLPC::initialize();
    });
    // clang-format on
}

void ObjCPlatformProxy::initialize()
{
    initializePlatform();
}

void ObjCPlatformProxy::initializePlatform()
{
    auto impl = currentProxyImpl();

    if (impl != nullptr)
    {
        [impl onInitializePlatform];
    }
}

void ObjCPlatformProxy::finalize()
{
    finalizePlatform();
}

void ObjCPlatformProxy::finalizePlatform()
{
    ObjCPlatformProxyImpl *impl = nullptr;

    {
        std::lock_guard<std::mutex> lock(proxyImplMutex);
        impl = proxyImpl;
        proxyImpl = nullptr;
    }

    if (impl != nullptr)
    {
        [impl onFinalizePlatform];
    }
}

void ObjCPlatformProxy::callProxy(const std::string &key, const std::string &data)
{
    auto impl = currentProxyImpl();

    if (impl != nullptr)
    {
        [impl onNativeProxyCall:ObjCSupport::xplpcCppStringToObjcString(key):ObjCSupport::xplpcCppStringToObjcString(data)];
        return;
    }

    // The caller is answered with the empty value rather than left waiting for a response that can no longer arrive.
    util::Log::e("[ObjCPlatformProxy : callProxy] The bridge is gone, so this call cannot reach swift");
    CallbackList::shared()->execute(key, "");
}

bool ObjCPlatformProxy::hasMapping(const std::string &name)
{
    auto impl = currentProxyImpl();

    if (impl != nullptr)
    {
        return [impl onHasMapping:ObjCSupport::xplpcCppStringToObjcString(name)];
    }

    return false;
}

void ObjCPlatformProxy::setProxyImpl(ObjCPlatformProxyImpl *proxyImpl)
{
    std::lock_guard<std::mutex> lock(proxyImplMutex);
    this->proxyImpl = proxyImpl;
}

ObjCPlatformProxyImpl *ObjCPlatformProxy::currentProxyImpl() const
{
    // A mapping may answer from a thread of its own, so the impl is taken under the lock and used through the local reference that keeps it alive.

    std::lock_guard<std::mutex> lock(proxyImplMutex);
    return proxyImpl;
}

} // namespace proxy
} // namespace xplpc
