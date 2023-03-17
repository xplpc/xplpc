#include "xplpc/proxy/CNativePlatformProxy.hpp"

namespace xplpc
{
namespace proxy
{

std::shared_ptr<CNativePlatformProxy> CNativePlatformProxy::instance = nullptr;

std::shared_ptr<CNativePlatformProxy> CNativePlatformProxy::shared()
{
    if (instance == nullptr)
    {
        instance = std::make_shared<CNativePlatformProxy>();
    }

    return instance;
}

void CNativePlatformProxy::callProxy(const std::string &key, const std::string &data)
{
    if (funcPtrToOnNativeProxyCall)
    {
        funcPtrToOnNativeProxyCall(const_cast<char *>(key.c_str()), key.size(), const_cast<char *>(data.c_str()), data.size());
    }
}

void CNativePlatformProxy::finalizePlatform()
{
    NativePlatformProxy::finalizePlatform();

    this->funcPtrToCallProxyCallback = nullptr;
    this->funcPtrToOnNativeProxyCall = nullptr;
}

void CNativePlatformProxy::setFuncPtrToOnNativeProxyCall(FuncPtrToOnNativeProxyCall funcPtrToOnNativeProxyCall)
{
    this->funcPtrToOnNativeProxyCall = funcPtrToOnNativeProxyCall;
}

FuncPtrToCallProxyCallback CNativePlatformProxy::getFuncPtrToCallProxyCallback()
{
    return this->funcPtrToCallProxyCallback;
}

FuncPtrToOnNativeProxyCall CNativePlatformProxy::getFuncPtrToOnNativeProxyCall()
{
    return this->funcPtrToOnNativeProxyCall;
}

} // namespace proxy
} // namespace xplpc
