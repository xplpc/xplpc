#ifndef XPLPC_NATIVE_PLATFORM_PROXY_IMPL_H
#define XPLPC_NATIVE_PLATFORM_PROXY_IMPL_H

#import <Foundation/Foundation.h>

@protocol XPLPCNativePlatformProxyProtocol
@required
- (NSString *)call:(NSString *)data;
@optional
- (NSString *)onRemoteClientCall:(NSString *)data;
@end

@interface XPLPCNativePlatformProxyImpl : NSObject <XPLPCNativePlatformProxyProtocol>
{
    // instance variables
}

// methods

@end

#endif
