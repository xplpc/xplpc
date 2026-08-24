#import "xplpc/client/Client.hpp"
#import "xplpc/data/CallbackList.hpp"
#import "xplpc/objc/support.h"
#import "xplpc/util/Log.hpp"
#import "xplpc/util/NativeBoundary.hpp"
#import <xplpc/ObjCPlatformProxyImpl.h>

#include <string>

using namespace xplpc::client;
using namespace xplpc::data;
using namespace xplpc::util;

@implementation ObjCPlatformProxyImpl

// The bridge calls these on whatever was registered, so a subclass that leaves one out is answered here rather than through an unrecognized selector.

// clang-format off
- (void)onNativeProxyCall:(NSString *)key :(NSString *)data
{
    (void)data;

    Log::e("[ObjCPlatformProxyImpl : onNativeProxyCall] The registered proxy does not implement this");
    CallbackList::shared()->execute(ObjCSupport::xplpcObjcStringToCppString(key), "");
}
// clang-format on

// clang-format off
- (void)onNativeProxyCallback:(NSString *)key :(NSString *)data
{
    (void)key;
    (void)data;

    Log::e("[ObjCPlatformProxyImpl : onNativeProxyCallback] The registered proxy does not implement this");
}
// clang-format on

- (bool)onHasMapping:(NSString *)name
{
    (void)name;

    Log::e("[ObjCPlatformProxyImpl : onHasMapping] The registered proxy does not implement this");

    return false;
}

- (void)onInitializePlatform
{
    Log::e("[ObjCPlatformProxyImpl : onInitializePlatform] The registered proxy does not implement this");
}

- (void)onFinalizePlatform
{
    Log::e("[ObjCPlatformProxyImpl : onFinalizePlatform] The registered proxy does not implement this");
}

// clang-format off
- (void)callNativeProxy:(NSString *)key :(NSString *)data
{
    NativeBoundary::run("callNativeProxy", [self, key, data] {
        auto convertedKey = ObjCSupport::xplpcObjcStringToCppString(key);

        // The receiver is captured weakly so a callback resolved after deallocation becomes a no-op instead of a dangling access.
        __weak ObjCPlatformProxyImpl *weakSelf = self;

        Client::call(ObjCSupport::xplpcObjcStringToCppString(data), [convertedKey, weakSelf](const std::string &response) {
            ObjCPlatformProxyImpl *strongSelf = weakSelf;

            if (!strongSelf)
            {
                Log::e("[ObjCPlatformProxyImpl : callNativeProxy] The bridge is gone, so this answer is lost");
                return;
            }

            [strongSelf onNativeProxyCallback:ObjCSupport::xplpcCppStringToObjcString(convertedKey) :ObjCSupport::xplpcCppStringToObjcString(response)];
        });
    });
}
// clang-format on

// clang-format off
- (void)callNativeProxyCallback:(NSString *)key :(NSString *)data
{
    NativeBoundary::run("callNativeProxyCallback", [key, data] {
        CallbackList::shared()->execute(ObjCSupport::xplpcObjcStringToCppString(key), ObjCSupport::xplpcObjcStringToCppString(data));
    });
}
// clang-format on

@end
