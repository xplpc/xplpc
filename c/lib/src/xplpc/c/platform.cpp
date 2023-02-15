#include "xplpc/c/platform.h"
#include "xplpc/client/ProxyClient.hpp"
#include "xplpc/core/XPLPC.hpp"
#include "xplpc/proxy/CPlatformProxy.hpp"
#include "xplpc/proxy/PlatformProxy.hpp"

#include <memory>

using namespace xplpc::client;
using namespace xplpc::core;
using namespace xplpc::proxy;

void xplpc_core_initialize(FuncPtrToCallProxyCallback funcPtrToCallProxyCallback, FuncPtrToOnNativeProxyCall funcPtrToOnNativeProxyCall)
{
    auto proxy = std::make_shared<CPlatformProxy>();
    proxy->initializeNativePlatform(funcPtrToCallProxyCallback, funcPtrToOnNativeProxyCall);

    PlatformProxy::create(proxy);
    PlatformProxy::shared()->initialize();
}

void xplpc_core_finalize()
{
    PlatformProxy::shared()->finalize();
}

bool xplpc_core_is_initialized()
{
    return XPLPC::isInitialized();
}

void xplpc_native_call_proxy(char *key, size_t keySize, char *data, size_t dataSize)
{
    // clang-format off
    ProxyClient::call(data, [key, keySize](const auto &response) {
        auto platformProxy = std::static_pointer_cast<CPlatformProxy>(PlatformProxy::shared());
        auto callback = platformProxy->getFuncPtrToOnNativeProxyCall();

        if (callback)
        {
            callback(key, keySize, const_cast<char *>(response.c_str()), response.size());
        }
    });
    // clang-format on
}

void xplpc_native_call_proxy_callback(char *key, size_t keySize, char *data, size_t dataSize)
{
    PlatformProxy::shared()->callProxyCallback(key, data);
}
