#import "xplpc/proxy/XPLPCNativePlatformProxy.h"
#import "xplpc/objc/support.h"

namespace xplpc
{
namespace proxy
{

std::shared_ptr<XPLPCNativePlatformProxy> XPLPCNativePlatformProxy::createNativePlatform()
{
    return std::make_shared<XPLPCNativePlatformProxy>();
}

void XPLPCNativePlatformProxy::initializeNativePlatform(XPLPCNativePlatformProxyImpl *proxy)
{
    this->instance = proxy;
}

void XPLPCNativePlatformProxy::callProxy(const std::string &key, const std::string &data)
{
    if (instance == nullptr)
    {
        [instance onNativeProxyCall:ObjCSupport::xplpcCppStringToObjcString(""):ObjCSupport::xplpcCppStringToObjcString("")];
        return;
    }

    [instance onNativeProxyCall:ObjCSupport::xplpcCppStringToObjcString(key):ObjCSupport::xplpcCppStringToObjcString(data)];
}

} // namespace proxy
} // namespace xplpc
