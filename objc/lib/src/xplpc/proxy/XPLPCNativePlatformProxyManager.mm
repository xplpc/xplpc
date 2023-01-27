#import "xplpc/proxy/XPLPCNativePlatformProxy.h"
#import <xplpc/XPLPCNativePlatformProxyManager.h>

using namespace xplpc::proxy;

@implementation XPLPCNativePlatformProxyManager

// methods
+ (void)initialize:(XPLPCNativePlatformProxyImpl *)proxy
{
    const auto platformProxy = XPLPCNativePlatformProxy::createNativePlatform();
    platformProxy->initializeNativePlatform(proxy);
    PlatformProxy::create(platformProxy);
    PlatformProxy::shared()->initialize();
}

@end
