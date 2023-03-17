#pragma once

#include "xplpc/c/typedefs.h"
#include "xplpc/proxy/NativePlatformProxy.hpp"

#include <cstddef>
#include <memory>
#include <string>

namespace xplpc
{
namespace proxy
{

class CNativePlatformProxy : public NativePlatformProxy
{
public:
    static std::shared_ptr<CNativePlatformProxy> shared();

    virtual void callProxy(const std::string &key, const std::string &data) override;
    virtual void finalize() override;

    void initializeNativePlatform(FuncPtrToCallProxyCallback funcPtrToCallProxyCallback, FuncPtrToOnNativeProxyCall funcPtrToOnNativeProxyCall);
    void finalizeNativePlatform();

    FuncPtrToCallProxyCallback getFuncPtrToCallProxyCallback();
    FuncPtrToOnNativeProxyCall getFuncPtrToOnNativeProxyCall();

private:
    static std::shared_ptr<CNativePlatformProxy> instance;

    FuncPtrToCallProxyCallback funcPtrToCallProxyCallback;
    FuncPtrToOnNativeProxyCall funcPtrToOnNativeProxyCall;
};

} // namespace proxy
} // namespace xplpc
