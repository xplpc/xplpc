#ifndef XPLPC_OBJC_PLATFORM_PROXY_IMPL_H
#define XPLPC_OBJC_PLATFORM_PROXY_IMPL_H

#import <Foundation/Foundation.h>

// clang-format off
@protocol ObjCPlatformProxyProtocol
@optional
- (void)onNativeProxyCallback:(NSString *)key :(NSString *)data;
- (void)onNativeProxyCall:(NSString *)key :(NSString *)data;
- (bool)onHasMapping:(NSString *)name;
- (void)onInitializePlatform;
- (void)onFinalizePlatform;
@required
- (void)callNativeProxy:(NSString *)key :(NSString *)data;
- (void)callNativeProxyCallback:(NSString *)key :(NSString *)data;
@end
// clang-format on

@interface ObjCPlatformProxyImpl : NSObject <ObjCPlatformProxyProtocol>
{
    // instance variables
}

// methods

@end

#endif
