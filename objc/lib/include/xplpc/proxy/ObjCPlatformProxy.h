#pragma once

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
    static void registerProxies();

    virtual void initialize() override;
    virtual void initializePlatform() override;
    void finalize();
    void finalizePlatform();
    virtual void callProxy(const std::string &key, const std::string &data) override;
    virtual bool hasMapping(const std::string &name) override;

    void setProxyImpl(ObjCPlatformProxyImpl *proxyImpl);

private:
    ObjCPlatformProxyImpl *currentProxyImpl() const;

    ObjCPlatformProxyImpl *proxyImpl = nullptr;
    mutable std::mutex proxyImplMutex;
    static std::shared_ptr<ObjCPlatformProxy> instance;
    static std::once_flag initInstanceFlag;
    static std::once_flag proxyRegistrationFlag;

    ObjCPlatformProxy() = default;
    ObjCPlatformProxy(const ObjCPlatformProxy &) = delete;
    ObjCPlatformProxy &operator=(const ObjCPlatformProxy &) = delete;
};

} // namespace proxy
} // namespace xplpc
