#import "xplpc/client/ProxyClient.hpp"
#import "xplpc/objc/support.h"
#import "xplpc/proxy/PlatformProxy.hpp"
#import <xplpc/XPLPCNativePlatformProxyImpl.h>

using namespace xplpc::client;
using namespace xplpc::proxy;

@implementation XPLPCNativePlatformProxyImpl

// methods

// clang-format off
- (void)nativeProxyCall:(NSString *)key :(NSString *)data
{
    auto convertedKey = ObjCSupport::xplpcObjcStringToCppString(key);

    ProxyClient::call(ObjCSupport::xplpcObjcStringToCppString(data), [convertedKey, &self](const auto &response) {
        if (self)
        {
            [self callProxyCallback:ObjCSupport::xplpcCppStringToObjcString(convertedKey) :ObjCSupport::xplpcCppStringToObjcString(response)];
        }
    });
}
// clang-format on

// clang-format off
- (void)nativeCallProxyCallback:(NSString *)key :(NSString *)data
{
    PlatformProxy::shared()->callProxyCallback(ObjCSupport::xplpcObjcStringToCppString(key), ObjCSupport::xplpcObjcStringToCppString(data));
}
// clang-format on

@end
