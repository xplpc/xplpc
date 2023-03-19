#ifndef XPLPC_OBJC_PLATFORM_PROXY_MANAGER_H
#define XPLPC_OBJC_PLATFORM_PROXY_MANAGER_H

#import <Foundation/Foundation.h>
#import <xplpc/ObjCPlatformProxyImpl.h>

@interface ObjCPlatformProxyManager : NSObject
{
    // instance variables
}

// methods

+ (void)initialize:(ObjCPlatformProxyImpl *)proxy;

@end

#endif
