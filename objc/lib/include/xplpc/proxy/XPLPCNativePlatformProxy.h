#ifndef XPLPC_NATIVE_PLATFORM_PROXY_H
#define XPLPC_NATIVE_PLATFORM_PROXY_H

#import "xplpc/proxy/PlatformProxy.hpp"
#import "xplpc/proxy/XPLPCNativePlatformProxyImpl.h"

#include <memory>
#include <string>

namespace xplpc
{
namespace proxy
{

class XPLPCNativePlatformProxy : public PlatformProxy
{
public:
    static std::shared_ptr<XPLPCNativePlatformProxy> createNativePlatform();
    void initializeNativePlatform(XPLPCNativePlatformProxyImpl *proxy);
    virtual void callProxy(const std::string &key, const std::string &data) override;

private:
    XPLPCNativePlatformProxyImpl *instance;
};

} // namespace proxy
} // namespace xplpc

#endif
