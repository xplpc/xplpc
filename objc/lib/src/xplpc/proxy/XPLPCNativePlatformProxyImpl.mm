#import "xplpc/client/ProxyClient.hpp"
#import "xplpc/objc/support.h"
#import <xplpc/XPLPCNativePlatformProxyImpl.h>

using namespace xplpc::client;

@implementation XPLPCNativePlatformProxyImpl

// methods
- (NSString *)call:(NSString *)data
{
    auto convertedData = ObjCSupport::xplpcObjcStringToCppString(std::move(data));
    return ObjCSupport::xplpcCppStringToObjcString(ProxyClient::call(std::move(convertedData)));
}

@end
