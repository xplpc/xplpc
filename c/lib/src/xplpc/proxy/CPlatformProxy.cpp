#include "xplpc/proxy/CPlatformProxy.hpp"

namespace xplpc
{
namespace proxy
{

void CPlatformProxy::callProxy(const std::string &key, const std::string &data)
{
    if (funcPtrToOnNativeProxyCall)
    {
        funcPtrToOnNativeProxyCall(const_cast<char *>(key.c_str()), key.size(), const_cast<char *>(data.c_str()), data.size());
    }
}

void CPlatformProxy::finalize()
{
    PlatformProxy::finalize();
    finalizeNativePlatform();
}

void CPlatformProxy::initializeNativePlatform(FuncPtrToCallProxyCallback funcPtrToCallProxyCallback, FuncPtrToOnNativeProxyCall funcPtrToOnNativeProxyCall)
{
    this->funcPtrToCallProxyCallback = funcPtrToCallProxyCallback;
    this->funcPtrToOnNativeProxyCall = funcPtrToOnNativeProxyCall;
}

void CPlatformProxy::finalizeNativePlatform()
{
    this->funcPtrToCallProxyCallback = nullptr;
    this->funcPtrToOnNativeProxyCall = nullptr;
}

FuncPtrToCallProxyCallback CPlatformProxy::getFuncPtrToCallProxyCallback()
{
    return this->funcPtrToCallProxyCallback;
}

FuncPtrToOnNativeProxyCall CPlatformProxy::getFuncPtrToOnNativeProxyCall()
{
    return this->funcPtrToOnNativeProxyCall;
}

} // namespace proxy
} // namespace xplpc
