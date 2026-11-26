#pragma once

#import "xplpc/core/Handle.hpp"
#import "xplpc/core/Layer.hpp"
#import <xplpc/ObjCLayerHost.h>

#include <atomic>
#include <string>

namespace xplpc
{
namespace layer
{

class ObjCLayer : public core::Layer
{
public:
    explicit ObjCLayer(id<ObjCLayerHost> host);

    void invoke(core::Handle call, const std::string &name, const std::string &request) override;

    static core::Handle attachedHandle();
    static bool publish(core::Handle handle);

private:
    // The host is held weakly, so a late invoke promotes it and finds nothing rather than reaching a deallocated object.
    __weak id<ObjCLayerHost> host;

    static std::atomic<core::Handle> handle;
};

} // namespace layer
} // namespace xplpc
