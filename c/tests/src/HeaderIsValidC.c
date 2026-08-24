/* A host reaching this library through the abi writes plain c, so the public headers are compiled as c here rather than only as c++. */

#include "xplpc/c/platform.h"
#include "xplpc/c/typedefs.h"

#include <stdbool.h>
#include <stddef.h>

typedef void (*AnyEntryPoint)(void);

static void onInitializePlatform(void)
{
}

static void onFinalizePlatform(void)
{
}

static void onBorrowedBuffers(const char *key, size_t keySize, const char *data, size_t dataSize)
{
    (void)key;
    (void)keySize;
    (void)data;
    (void)dataSize;
}

static void onOwnedBuffers(char *key, size_t keySize, char *data, size_t dataSize)
{
    (void)keySize;
    (void)dataSize;

    xplpc_free(key);
    xplpc_free(data);
}

size_t xplpcCEntryPointCount(void);

size_t xplpcCEntryPointCount(void)
{
    const FuncPtrToOnInitializePlatform initializePlatform = onInitializePlatform;
    const FuncPtrToOnFinalizePlatform finalizePlatform = onFinalizePlatform;
    const FuncPtrToOnNativeProxyCall nativeProxyCall = onBorrowedBuffers;
    const FuncPtrToOnNativeProxyCallback nativeProxyCallback = onBorrowedBuffers;
    const FuncPtrToOnNativeProxyCallFromThread nativeProxyCallFromThread = onOwnedBuffers;
    const FuncPtrToOnNativeProxyCallbackFromThread nativeProxyCallbackFromThread = onOwnedBuffers;

    const AnyEntryPoint entryPoints[] = {
        (AnyEntryPoint)xplpc_core_initialize,
        (AnyEntryPoint)xplpc_core_finalize,
        (AnyEntryPoint)xplpc_core_is_initialized,
        (AnyEntryPoint)xplpc_native_call_proxy,
        (AnyEntryPoint)xplpc_native_call_proxy_callback,
        (AnyEntryPoint)xplpc_native_add_mapping,
        (AnyEntryPoint)xplpc_native_clear_mappings,
        (AnyEntryPoint)xplpc_free,
    };

    if (!initializePlatform || !finalizePlatform || !nativeProxyCall || !nativeProxyCallback || !nativeProxyCallFromThread || !nativeProxyCallbackFromThread)
    {
        return 0;
    }

    return sizeof(entryPoints) / sizeof(entryPoints[0]);
}
