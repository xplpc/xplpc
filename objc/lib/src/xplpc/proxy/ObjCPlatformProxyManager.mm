#import "xplpc/proxy/ObjCPlatformProxy.h"
#import <xplpc/ObjCPlatformProxyManager.h>

using namespace xplpc::proxy;

@implementation ObjCPlatformProxyManager

+ (void)registerProxy:(ObjCPlatformProxyImpl *)proxyImpl
{
    auto objcPlatformProxy = ObjCPlatformProxy::shared();
    objcPlatformProxy->setProxyImpl(proxyImpl);

    ObjCPlatformProxy::registerProxies();

    objcPlatformProxy->initialize();
}

@end
