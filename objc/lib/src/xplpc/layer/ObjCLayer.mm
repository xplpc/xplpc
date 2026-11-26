#import "xplpc/layer/ObjCLayer.h"
#import "xplpc/core/Router.hpp"
#import "xplpc/objc/support.h"
#import "xplpc/util/Log.hpp"

namespace xplpc
{
namespace layer
{

std::atomic<core::Handle> ObjCLayer::handle{0};

ObjCLayer::ObjCLayer(id<ObjCLayerHost> host)
    : host(host)
{
}

void ObjCLayer::invoke(core::Handle call, const std::string & /*name*/, const std::string &request)
{
    id<ObjCLayerHost> strongHost = host;

    if (!strongHost)
    {
        util::Log::e("[ObjCLayer : invoke] The swift layer is gone, so this call cannot reach it");
        core::Router::shared().answer(call, "");
        return;
    }

    [strongHost invoke:call request:ObjCSupport::xplpcCppStringToObjcString(request)];
}

core::Handle ObjCLayer::attachedHandle()
{
    return handle.load(std::memory_order_acquire);
}

bool ObjCLayer::publish(core::Handle attachedHandle)
{
    core::Handle expected = 0;
    return handle.compare_exchange_strong(expected, attachedHandle, std::memory_order_acq_rel);
}

} // namespace layer
} // namespace xplpc
