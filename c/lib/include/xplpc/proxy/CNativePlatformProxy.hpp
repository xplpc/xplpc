#pragma once

#include "xplpc/c/typedefs.h"
#include "xplpc/proxy/PlatformProxy.hpp"

#include <cstddef>
#include <memory>
#include <mutex>
#include <string>

namespace xplpc
{
namespace proxy
{

class CNativePlatformProxy : public PlatformProxy
{
public:
    static std::shared_ptr<CNativePlatformProxy> shared();

    virtual void initialize() override;
    virtual void initializePlatform() override;
    virtual void finalize() override;
    virtual void finalizePlatform() override;
    virtual void callProxy(const std::string &key, const std::string &data) override;
    virtual bool hasMapping(const std::string &name) override;

    void setFuncPtrToOnInitializePlatform(FuncPtrToOnInitializePlatform funcPtrToOnInitializePlatform);
    void setFuncPtrToOnFinalizePlatform(FuncPtrToOnFinalizePlatform funcPtrToOnFinalizePlatform);
    void setFuncPtrToOnHasMapping(FuncPtrToOnHasMapping funcPtrToOnHasMapping);
    void setFuncPtrToOnNativeProxyCall(FuncPtrToOnNativeProxyCall funcPtrToOnNativeProxyCall);
    void setFuncPtrToOnNativeProxyCallback(FuncPtrToOnNativeProxyCallback funcPtrToOnNativeProxyCallback);

    FuncPtrToOnInitializePlatform getFuncPtrToOnInitializePlatform();
    FuncPtrToOnFinalizePlatform getFuncPtrToOnFinalizePlatform();
    FuncPtrToOnHasMapping getFuncPtrToOnHasMapping();
    FuncPtrToOnNativeProxyCall getFuncPtrToOnNativeProxyCall();
    FuncPtrToOnNativeProxyCallback getFuncPtrToOnNativeProxyCallback();

private:
    static std::shared_ptr<CNativePlatformProxy> instance;
    static std::once_flag initInstanceFlag;

    FuncPtrToOnInitializePlatform funcPtrToOnInitializePlatform;
    FuncPtrToOnFinalizePlatform funcPtrToOnFinalizePlatform;
    FuncPtrToOnHasMapping funcPtrToOnHasMapping;
    FuncPtrToOnNativeProxyCall funcPtrToOnNativeProxyCall;
    FuncPtrToOnNativeProxyCallback funcPtrToOnNativeProxyCallback;

    CNativePlatformProxy() = default;
    CNativePlatformProxy(const CNativePlatformProxy &) = delete;
    CNativePlatformProxy &operator=(const CNativePlatformProxy &) = delete;
};

} // namespace proxy
} // namespace xplpc
