#import "xplpc/data/PlatformProxyList.hpp"
#import "xplpc/proxy/NativePlatformProxy.hpp"
#import "xplpc/proxy/ObjCPlatformProxy.h"
#import <xplpc/ObjCPlatformProxyManager.h>

using namespace xplpc::proxy;

@implementation ObjCPlatformProxyManager

// methods
+ (void)initialize:(ObjCPlatformProxyImpl *)proxyImpl
{
    // initialize cxx platform proxy
    auto nativePlatformProxy = std::make_shared<xplpc::proxy::NativePlatformProxy>();
    nativePlatformProxy->initialize();
    xplpc::data::PlatformProxyList::shared()->insert(0, nativePlatformProxy);

    // initialize objc platform proxy
    auto objcPlatformProxy = ObjCPlatformProxy::shared();
    objcPlatformProxy->setProxyImpl(proxyImpl);
    objcPlatformProxy->initialize();

    xplpc::data::PlatformProxyList::shared()->insert(0, objcPlatformProxy);
}

@end
