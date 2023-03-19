#import "xplpc/client/Client.hpp"
#import "xplpc/data/CallbackList.hpp"
#import "xplpc/objc/support.h"
#import <xplpc/ObjCPlatformProxyImpl.h>

using namespace xplpc::client;
using namespace xplpc::data;

@implementation ObjCPlatformProxyImpl

// methods

// clang-format off
- (void)callNativeProxy:(NSString *)key :(NSString *)data
{
    auto convertedKey = ObjCSupport::xplpcObjcStringToCppString(key);

    Client::call(ObjCSupport::xplpcObjcStringToCppString(data), [convertedKey, &self](const auto &response) {
        if (self)
        {
            [self onNativeProxyCallback:ObjCSupport::xplpcCppStringToObjcString(convertedKey) :ObjCSupport::xplpcCppStringToObjcString(response)];
        }
    });
}
// clang-format on

// clang-format off
- (void)callNativeProxyCallback:(NSString *)key :(NSString *)data
{
    CallbackList::shared()->execute(ObjCSupport::xplpcObjcStringToCppString(key), ObjCSupport::xplpcObjcStringToCppString(data));
}
// clang-format on

@end
