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

std::string XPLPCNativePlatformProxy::call(const std::string &data)
{
    if (instance == nullptr)
    {
        return ObjCSupport::xplpcObjcStringToCppString(ObjCSupport::xplpcCppStringToObjcString(""));
    }

    auto convertedData = ObjCSupport::xplpcCppStringToObjcString(data);
    return ObjCSupport::xplpcObjcStringToCppString([instance onRemoteClientCall:convertedData]);
}

} // namespace proxy
} // namespace xplpc
