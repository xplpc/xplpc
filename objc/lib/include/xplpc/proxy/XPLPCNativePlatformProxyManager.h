#ifndef XPLPC_NATIVE_PLATFORM_PROXY_MANAGER_H
#define XPLPC_NATIVE_PLATFORM_PROXY_MANAGER_H

#import <Foundation/Foundation.h>
#import <xplpc/XPLPCNativePlatformProxyImpl.h>

@interface XPLPCNativePlatformProxyManager : NSObject
{
    // instance variables
}

// methods

+ (void)initialize:(XPLPCNativePlatformProxyImpl *)proxy;

@end

#endif
