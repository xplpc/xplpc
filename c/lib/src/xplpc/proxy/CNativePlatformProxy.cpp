#include "xplpc/proxy/CNativePlatformProxy.hpp"

namespace xplpc
{
namespace proxy
{

std::shared_ptr<CNativePlatformProxy> CNativePlatformProxy::instance = nullptr;
std::once_flag CNativePlatformProxy::initInstanceFlag;

std::shared_ptr<CNativePlatformProxy> CNativePlatformProxy::shared()
{
    // clang-format off
    std::call_once(initInstanceFlag, []() {
        instance = std::shared_ptr<CNativePlatformProxy>(new CNativePlatformProxy());
    });
    // clang-format on

    return instance;
}

void CNativePlatformProxy::initialize()
{
    initializePlatform();
}

void CNativePlatformProxy::initializePlatform()
{
    if (funcPtrToOnInitializePlatform)
    {
        funcPtrToOnInitializePlatform();
    }
}

void CNativePlatformProxy::finalize()
{
    finalizePlatform();
}

void CNativePlatformProxy::finalizePlatform()
{
    if (funcPtrToOnFinalizePlatform)
    {
        funcPtrToOnFinalizePlatform();
    }

    funcPtrToOnInitializePlatform = nullptr;
    funcPtrToOnFinalizePlatform = nullptr;
    funcPtrToOnHasMapping = nullptr;
    funcPtrToOnNativeProxyCall = nullptr;
    funcPtrToOnNativeProxyCallback = nullptr;
}

bool CNativePlatformProxy::hasMapping(const std::string &name)
{
    if (funcPtrToOnHasMapping)
    {
        return funcPtrToOnHasMapping(const_cast<char *>(name.c_str()), name.size());
    }

    return false;
}

void CNativePlatformProxy::callProxy(const std::string &key, const std::string &data)
{
    if (funcPtrToOnNativeProxyCall)
    {
        funcPtrToOnNativeProxyCall(const_cast<char *>(key.c_str()), key.size(), const_cast<char *>(data.c_str()), data.size());
    }
}

void CNativePlatformProxy::setFuncPtrToOnInitializePlatform(FuncPtrToOnInitializePlatform funcPtrToOnInitializePlatform)
{
    this->funcPtrToOnInitializePlatform = funcPtrToOnInitializePlatform;
}

void CNativePlatformProxy::setFuncPtrToOnFinalizePlatform(FuncPtrToOnFinalizePlatform funcPtrToOnFinalizePlatform)
{
    this->funcPtrToOnFinalizePlatform = funcPtrToOnFinalizePlatform;
}

void CNativePlatformProxy::setFuncPtrToOnHasMapping(FuncPtrToOnHasMapping funcPtrToOnHasMapping)
{
    this->funcPtrToOnHasMapping = funcPtrToOnHasMapping;
}

void CNativePlatformProxy::setFuncPtrToOnNativeProxyCall(FuncPtrToOnNativeProxyCall funcPtrToOnNativeProxyCall)
{
    this->funcPtrToOnNativeProxyCall = funcPtrToOnNativeProxyCall;
}

void CNativePlatformProxy::setFuncPtrToOnNativeProxyCallback(FuncPtrToOnNativeProxyCallback funcPtrToOnNativeProxyCallback)
{
    this->funcPtrToOnNativeProxyCallback = funcPtrToOnNativeProxyCallback;
}

FuncPtrToOnInitializePlatform CNativePlatformProxy::getFuncPtrToOnInitializePlatform()
{
    return this->funcPtrToOnInitializePlatform;
}

FuncPtrToOnFinalizePlatform CNativePlatformProxy::getFuncPtrToOnFinalizePlatform()
{
    return this->funcPtrToOnFinalizePlatform;
}

FuncPtrToOnHasMapping CNativePlatformProxy::getFuncPtrToOnHasMapping()
{
    return this->funcPtrToOnHasMapping;
}

FuncPtrToOnNativeProxyCall CNativePlatformProxy::getFuncPtrToOnNativeProxyCall()
{
    return this->funcPtrToOnNativeProxyCall;
}

FuncPtrToOnNativeProxyCallback CNativePlatformProxy::getFuncPtrToOnNativeProxyCallback()
{
    return this->funcPtrToOnNativeProxyCallback;
}

} // namespace proxy
} // namespace xplpc
