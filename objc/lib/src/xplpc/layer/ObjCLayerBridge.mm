#import "xplpc/core/BufferTable.hpp"
#import "xplpc/core/CallState.hpp"
#import "xplpc/core/Router.hpp"
#import "xplpc/layer/ObjCLayer.h"
#import "xplpc/objc/support.h"
#import "xplpc/util/NativeBoundary.hpp"
#import <xplpc/ObjCLayerBridge.h>

#include <cstdint>
#include <memory>
#include <optional>
#include <string>

using namespace xplpc::core;
using namespace xplpc::layer;
using namespace xplpc::util;

@implementation ObjCLayerBridge

+ (BOOL)attach:(id<ObjCLayerHost>)host
{
    // Attaching runs the mappings a consumer declares in c++ the first time, so whatever they raise stops here instead of unwinding into swift.

    // clang-format off
    return NativeBoundary::runAnswering("attach", NO, [host] {
        if (ObjCLayer::attachedHandle())
        {
            return YES;
        }

        const auto handle = Router::shared().attach(std::make_shared<ObjCLayer>(host));

        if (!ObjCLayer::publish(handle))
        {
            Router::shared().detach(handle);
        }

        return YES;
    });
    // clang-format on
}

+ (void)declare:(NSString *)name
{
    // clang-format off
    NativeBoundary::run("declare", [name] {
        Router::shared().declare(ObjCLayer::attachedHandle(), ObjCSupport::xplpcObjcStringToCppString(name));
    });
    // clang-format on
}

+ (void)retract:(NSString *)name
{
    // clang-format off
    NativeBoundary::run("retract", [name] {
        Router::shared().retract(ObjCLayer::attachedHandle(), ObjCSupport::xplpcObjcStringToCppString(name));
    });
    // clang-format on
}

+ (uint64_t)call:(NSString *)request answer:(void (^)(NSString *response))answer
{
    // clang-format off
    const Completion completion = [answer](const std::string &response) {
        answer(ObjCSupport::xplpcCppStringToObjcString(response));
    };

    const auto routed = NativeBoundary::runAnswering("call", std::optional<CallResult>{}, [request, &completion] {
        return std::optional<CallResult>{Router::shared().call(ObjCLayer::attachedHandle(), ObjCSupport::xplpcObjcStringToCppString(request), completion)};
    });
    // clang-format on

    // A call stopped at the boundary never reached the router, so nothing else would ever answer it.
    if (!routed)
    {
        CallState::run(completion, "");
        return 0;
    }

    if (routed->isAnswered())
    {
        CallState::run(completion, routed->value());
    }

    return routed->handle();
}

+ (BOOL)cancel:(uint64_t)call
{
    // clang-format off
    return NativeBoundary::runAnswering("cancel", NO, [call] {
        return Router::shared().cancel(ObjCLayer::attachedHandle(), call) ? YES : NO;
    });
    // clang-format on
}

+ (void)answer:(uint64_t)call response:(NSString *)response
{
    // clang-format off
    NativeBoundary::run("answer", [call, response] {
        Router::shared().answer(call, ObjCSupport::xplpcObjcStringToCppString(response));
    });
    // clang-format on
}

+ (uint64_t)registerBuffer:(void *)data size:(NSUInteger)size
{
    // clang-format off
    return NativeBoundary::runAnswering("registerBuffer", uint64_t{0}, [data, size] {
        return BufferTable::shared().add(ObjCLayer::attachedHandle(), static_cast<std::uint8_t *>(data), size);
    });
    // clang-format on
}

+ (BOOL)releaseBuffer:(uint64_t)buffer
{
    // clang-format off
    return NativeBoundary::runAnswering("releaseBuffer", NO, [buffer] {
        return BufferTable::shared().release(ObjCLayer::attachedHandle(), buffer) ? YES : NO;
    });
    // clang-format on
}

+ (nullable NSData *)readBuffer:(uint64_t)buffer
{
    // clang-format off
    return NativeBoundary::runAnswering("readBuffer", static_cast<NSData *>(nil), [buffer]() -> NSData * {
        const auto span = BufferTable::shared().read(buffer);

        if (!span)
        {
            return nil;
        }

        return [NSData dataWithBytes:span->data length:span->size];
    });
    // clang-format on
}

+ (NSUInteger)pending
{
    // clang-format off
    return NativeBoundary::runAnswering("pending", NSUInteger{0}, [] {
        return static_cast<NSUInteger>(Router::shared().pending());
    });
    // clang-format on
}

@end
