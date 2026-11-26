#ifndef XPLPC_OBJC_LAYER_HOST_H
#define XPLPC_OBJC_LAYER_HOST_H

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

// Swift as one layer of the router, which serves a call for a name it declared and answers it through the bridge with the handle it received.
@protocol ObjCLayerHost
@required
- (void)invoke:(uint64_t)call request:(NSString *)request;
@end

NS_ASSUME_NONNULL_END

#endif
