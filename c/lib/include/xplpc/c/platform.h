#ifndef XPLPC_C_PLATFORM_H
#define XPLPC_C_PLATFORM_H

#include "xplpc/c/typedefs.h"

#if defined(_WIN32)
#define XPLPC_API __declspec(dllexport)
#else
#define XPLPC_API __attribute__((visibility("default"))) __attribute__((used))
#endif

#ifdef __cplusplus
extern "C"
{
#endif

    XPLPC_API
    void xplpc_core_initialize(
        bool initializeCxxNativePlatformProxy,
        FuncPtrToOnInitializePlatform funcPtrToOnInitializePlatform,
        FuncPtrToOnFinalizePlatform funcPtrToOnFinalizePlatform,
        FuncPtrToOnNativeProxyCall funcPtrToOnNativeProxyCall,
        FuncPtrToOnNativeProxyCallback funcPtrToOnNativeProxyCallback,
        FuncPtrToOnNativeProxyCallFromThread funcPtrToOnNativeProxyCallFromThread,
        FuncPtrToOnNativeProxyCallbackFromThread funcPtrToOnNativeProxyCallbackFromThread);

    XPLPC_API
    void xplpc_core_finalize(void);

    XPLPC_API
    bool xplpc_core_is_initialized(void);

    XPLPC_API
    void xplpc_native_call_proxy(const char *key, size_t keySize, const char *data, size_t dataSize);

    XPLPC_API
    void xplpc_native_call_proxy_callback(const char *key, size_t keySize, const char *data, size_t dataSize);

    XPLPC_API
    void xplpc_native_add_mapping(const char *name, size_t nameSize);

    XPLPC_API
    void xplpc_native_clear_mappings(void);

    XPLPC_API
    void xplpc_free(void *ptr);

#ifdef __cplusplus
}
#endif

#endif
