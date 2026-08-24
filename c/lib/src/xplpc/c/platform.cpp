#include "xplpc/c/platform.h"
#include "xplpc/client/Client.hpp"
#include "xplpc/core/XPLPC.hpp"
#include "xplpc/data/CallbackList.hpp"
#include "xplpc/proxy/CNativePlatformProxy.hpp"
#include "xplpc/proxy/HostCallScope.hpp"
#include "xplpc/util/Log.hpp"
#include "xplpc/util/NativeBoundary.hpp"

#include <cstdlib>
#include <string>

using namespace xplpc::client;
using namespace xplpc::core;
using namespace xplpc::data;
using namespace xplpc::proxy;
using namespace xplpc::util;

void xplpc_core_initialize(
    bool initializeCxxNativePlatformProxy,
    FuncPtrToOnInitializePlatform funcPtrToOnInitializePlatform,
    FuncPtrToOnFinalizePlatform funcPtrToOnFinalizePlatform,
    FuncPtrToOnNativeProxyCall funcPtrToOnNativeProxyCall,
    FuncPtrToOnNativeProxyCallback funcPtrToOnNativeProxyCallback,
    FuncPtrToOnNativeProxyCallFromThread funcPtrToOnNativeProxyCallFromThread,
    FuncPtrToOnNativeProxyCallbackFromThread funcPtrToOnNativeProxyCallbackFromThread)
{
    // clang-format off
    NativeBoundary::run("xplpc_core_initialize", [&] {
        auto cPlatformProxy = CNativePlatformProxy::shared();
        cPlatformProxy->setFuncPtrToOnInitializePlatform(funcPtrToOnInitializePlatform);
        cPlatformProxy->setFuncPtrToOnFinalizePlatform(funcPtrToOnFinalizePlatform);
        cPlatformProxy->setFuncPtrToOnNativeProxyCall(funcPtrToOnNativeProxyCall);
        cPlatformProxy->setFuncPtrToOnNativeProxyCallback(funcPtrToOnNativeProxyCallback);
        cPlatformProxy->setFuncPtrToOnNativeProxyCallFromThread(funcPtrToOnNativeProxyCallFromThread);
        cPlatformProxy->setFuncPtrToOnNativeProxyCallbackFromThread(funcPtrToOnNativeProxyCallbackFromThread);

        CNativePlatformProxy::registerProxies(initializeCxxNativePlatformProxy);

        cPlatformProxy->initialize();

        // The logger and the flag belong to the core rather than to any proxy, and a host that wants none of the c++ mappings still initialized it.
        XPLPC::initialize();
    });
    // clang-format on
}

void xplpc_core_finalize()
{
    // clang-format off
    NativeBoundary::run("xplpc_core_finalize", [] {
        CNativePlatformProxy::shared()->finalize();
    });
    // clang-format on
}

bool xplpc_core_is_initialized()
{
    // clang-format off
    return NativeBoundary::runAnswering("xplpc_core_is_initialized", false, [] {
        return XPLPC::isInitialized();
    });
    // clang-format on
}

void xplpc_native_call_proxy(const char *key, size_t keySize, const char *data, size_t dataSize)
{
    if (!key || !data)
    {
        Log::e("[xplpc_native_call_proxy] The key or the data carries no address");
        return;
    }

    // clang-format off
    NativeBoundary::run("xplpc_native_call_proxy", [&] {
        // Both buffers are owned by the caller and may be released as soon as this returns, so they are copied before dispatching.
        const auto ownedKey = std::string(key, keySize);

        // The thread is marked while the call runs, so an answer produced on it reaches the host directly.
        HostCallScope scope;

        Client::call(std::string(data, dataSize), [ownedKey](const std::string &response) {
            CNativePlatformProxy::shared()->answer(ownedKey, response);
        });
    });
    // clang-format on
}

void xplpc_native_call_proxy_callback(const char *key, size_t keySize, const char *data, size_t dataSize)
{
    if (!key || !data)
    {
        Log::e("[xplpc_native_call_proxy_callback] The key or the data carries no address");
        return;
    }

    // clang-format off
    NativeBoundary::run("xplpc_native_call_proxy_callback", [&] {
        // The host is running on this thread, so anything the answer sets off can reach it directly.
        HostCallScope scope;

        CallbackList::shared()->execute(std::string(key, keySize), std::string(data, dataSize));
    });
    // clang-format on
}

void xplpc_native_add_mapping(const char *name, size_t nameSize)
{
    if (!name)
    {
        Log::e("[xplpc_native_add_mapping] The name carries no address");
        return;
    }

    // clang-format off
    NativeBoundary::run("xplpc_native_add_mapping", [&] {
        CNativePlatformProxy::shared()->addMapping(std::string(name, nameSize));
    });
    // clang-format on
}

void xplpc_native_clear_mappings()
{
    // clang-format off
    NativeBoundary::run("xplpc_native_clear_mappings", [] {
        CNativePlatformProxy::shared()->clearMappings();
    });
    // clang-format on
}

void xplpc_free(void *ptr)
{
    std::free(ptr);
}
