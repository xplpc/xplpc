#ifndef XPLPC_NATIVE_PLATFORM_PROXY_IMPL_H
#define XPLPC_NATIVE_PLATFORM_PROXY_IMPL_H

#import <Foundation/Foundation.h>

// clang-format off
@protocol XPLPCNativePlatformProxyProtocol
@required
- (void)nativeProxyCall:(NSString *)key :(NSString *)data;
- (void)nativeCallProxyCallback:(NSString *)key :(NSString *)data;
@optional
- (void)onNativeProxyCall:(NSString *)key :(NSString *)data;
- (void)callProxyCallback:(NSString *)key :(NSString *)data;
@end
// clang-format on

@interface XPLPCNativePlatformProxyImpl : NSObject <XPLPCNativePlatformProxyProtocol>
{
    // instance variables
}

// methods

@end

#endif
