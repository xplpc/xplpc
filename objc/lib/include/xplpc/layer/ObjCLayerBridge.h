#ifndef XPLPC_OBJC_LAYER_BRIDGE_H
#define XPLPC_OBJC_LAYER_BRIDGE_H

#import <Foundation/Foundation.h>
#import <xplpc/ObjCLayerHost.h>

NS_ASSUME_NONNULL_BEGIN

// The framework hides every symbol it does not mark, and a header it ships has to name a class that links.
__attribute__((visibility("default")))
@interface ObjCLayerBridge : NSObject

// Attaches the host once per process and holds it weakly, so an invoke that arrives after it is gone is answered with the empty value.
+ (BOOL)attach:(id<ObjCLayerHost>)host;

+ (void)declare:(NSString *)name;

+ (void)retract:(NSString *)name;

// Answers the handle of a call still waiting, or zero once the answer was handed over, which happens before this returns when the mapping answered inline.
+ (uint64_t)call:(NSString *)request answer:(void (^)(NSString *response))answer;

+ (BOOL)cancel:(uint64_t)call;

+ (void)answer:(uint64_t)call response:(NSString *)response;

// Registers bytes the caller keeps valid and in place until it releases the handle, which is the only thing that crosses the wire.
+ (uint64_t)registerBuffer:(void *)data size:(NSUInteger)size;

+ (BOOL)releaseBuffer:(uint64_t)buffer;

// Answers a copy of the bytes a handle names, or nothing when it names no registered buffer.
+ (nullable NSData *)readBuffer:(uint64_t)buffer;

// The number of calls still waiting anywhere in the process, which is what lets a suite prove it left none behind.
+ (NSUInteger)pending;

@end

NS_ASSUME_NONNULL_END

#endif
