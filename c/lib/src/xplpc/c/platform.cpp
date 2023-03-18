#include "xplpc/c/platform.h"
#include "xplpc/client/Client.hpp"
#include "xplpc/core/XPLPC.hpp"
#include "xplpc/data/PlatformProxyList.hpp"
#include "xplpc/proxy/CNativePlatformProxy.hpp"
#include "xplpc/proxy/PlatformProxy.hpp"

#include <memory>

using namespace xplpc::client;
using namespace xplpc::core;
using namespace xplpc::data;
using namespace xplpc::proxy;

void xplpc_core_initialize(FuncPtrToCallProxyCallback funcPtrToCallProxyCallback, FuncPtrToOnNativeProxyCall funcPtrToOnNativeProxyCall)
{
    auto proxy = CNativePlatformProxy::shared();
    proxy->setFuncPtrToOnNativeProxyCall(funcPtrToOnNativeProxyCall);
    proxy->initialize();

    PlatformProxyList::shared()->insert(0, proxy);
}

void xplpc_core_finalize()
{
    CNativePlatformProxy::shared()->finalize();
}

bool xplpc_core_is_initialized()
{
    return XPLPC::isInitialized();
}

void xplpc_native_call_proxy(char *key, size_t keySize, char *data, size_t dataSize)
{
    // clang-format off
    Client::call(data, [key, keySize](const auto &response) {
        auto callback = CNativePlatformProxy::shared()->getFuncPtrToCallProxyCallback();

        if (callback)
        {
            callback(key, keySize, const_cast<char *>(response.c_str()), response.size());
        }
    });
    // clang-format on
}
