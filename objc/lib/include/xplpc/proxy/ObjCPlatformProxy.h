#ifndef XPLPC_OBJC_PLATFORM_PROXY_H
#define XPLPC_OBJC_PLATFORM_PROXY_H

#import "xplpc/proxy/ObjCPlatformProxyImpl.h"
#import "xplpc/proxy/PlatformProxy.hpp"

#include <memory>
#include <mutex>
#include <string>

namespace xplpc
{
namespace proxy
{

class ObjCPlatformProxy : public PlatformProxy
{
public:
    static std::shared_ptr<ObjCPlatformProxy> shared();

    virtual void initialize() override;
    virtual void initializePlatform() override;
    virtual void finalize() override;
    virtual void finalizePlatform() override;
    virtual void callProxy(const std::string &key, const std::string &data) override;
    virtual bool hasMapping(const std::string &name) override;

    void setProxyImpl(ObjCPlatformProxyImpl *proxyImpl);

private:
    ObjCPlatformProxyImpl *proxyImpl;
    static std::shared_ptr<ObjCPlatformProxy> instance;
    static std::once_flag initInstanceFlag;

    ObjCPlatformProxy() = default;
    ObjCPlatformProxy(const ObjCPlatformProxy &) = delete;
    ObjCPlatformProxy &operator=(const ObjCPlatformProxy &) = delete;
};

} // namespace proxy
} // namespace xplpc

#endif
