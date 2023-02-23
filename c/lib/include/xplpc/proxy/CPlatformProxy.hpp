#pragma once

#include "xplpc/c/typedefs.h"
#include "xplpc/proxy/PlatformProxy.hpp"

#include <cstddef>
#include <string>

namespace xplpc
{
namespace proxy
{

class CPlatformProxy : public PlatformProxy
{
public:
    virtual void callProxy(const std::string &key, const std::string &data) override;
    virtual void finalize() override;
    void initializeNativePlatform(FuncPtrToCallProxyCallback funcPtrToCallProxyCallback, FuncPtrToOnNativeProxyCall funcPtrToOnNativeProxyCall);
    void finalizeNativePlatform();
    FuncPtrToCallProxyCallback getFuncPtrToCallProxyCallback();
    FuncPtrToOnNativeProxyCall getFuncPtrToOnNativeProxyCall();

private:
    FuncPtrToCallProxyCallback funcPtrToCallProxyCallback;
    FuncPtrToOnNativeProxyCall funcPtrToOnNativeProxyCall;
};

} // namespace proxy
} // namespace xplpc
