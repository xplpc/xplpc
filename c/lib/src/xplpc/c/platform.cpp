#include "xplpc/c/platform.h"
#include "xplpc/client/Client.hpp"
#include "xplpc/core/XPLPC.hpp"
#include "xplpc/data/CallbackList.hpp"
#include "xplpc/data/PlatformProxyList.hpp"
#include "xplpc/proxy/CNativePlatformProxy.hpp"
#include "xplpc/proxy/NativePlatformProxy.hpp"
#include "xplpc/proxy/PlatformProxy.hpp"

#include <memory>
#include <string>

using namespace xplpc::client;
using namespace xplpc::core;
using namespace xplpc::data;
using namespace xplpc::proxy;

void xplpc_core_initialize(
    FuncPtrToOnInitializePlatform funcPtrToOnInitializePlatform,
    FuncPtrToOnFinalizePlatform funcPtrToOnFinalizePlatform,
    FuncPtrToOnHasMapping funcPtrToOnHasMapping,
    FuncPtrToOnNativeProxyCall funcPtrToOnNativeProxyCall,
    FuncPtrToOnNativeProxyCallback funcPtrToOnNativeProxyCallback)
{
    // initialize cxx platform proxy
    auto nativePlatformProxy = std::make_shared<NativePlatformProxy>();
    nativePlatformProxy->initialize();
    PlatformProxyList::shared()->insert(0, nativePlatformProxy);

    // initialize c platform proxy
    auto cPlatformProxy = CNativePlatformProxy::shared();
    cPlatformProxy->setFuncPtrToOnInitializePlatform(funcPtrToOnInitializePlatform);
    cPlatformProxy->setFuncPtrToOnFinalizePlatform(funcPtrToOnFinalizePlatform);
    cPlatformProxy->setFuncPtrToOnHasMapping(funcPtrToOnHasMapping);
    cPlatformProxy->setFuncPtrToOnNativeProxyCall(funcPtrToOnNativeProxyCall);
    cPlatformProxy->setFuncPtrToOnNativeProxyCallback(funcPtrToOnNativeProxyCallback);
    cPlatformProxy->initialize();

    PlatformProxyList::shared()->insert(0, cPlatformProxy);
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
        auto callback = CNativePlatformProxy::shared()->getFuncPtrToOnNativeProxyCallback();

        if (callback)
        {
            callback(key, keySize, const_cast<char *>(response.c_str()), response.size());
        }
    });
    // clang-format on
}

void xplpc_native_call_proxy_callback(char *key, size_t keySize, char *data, size_t dataSize)
{
    CallbackList::shared()->execute(std::string(key, keySize), std::string(data, dataSize));
}
