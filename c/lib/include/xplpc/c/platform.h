#ifndef XPLPC_C_PLATFORM_H
#define XPLPC_C_PLATFORM_H

#include "xplpc/c/typedefs.h"

#if _WIN32
#define XPLPC_EXPORT extern "C" __declspec(dllexport)
#else
#define XPLPC_EXPORT extern "C" __attribute__((visibility("default"))) __attribute__((used))
#endif

XPLPC_EXPORT
void xplpc_core_initialize(FuncPtrToCallProxyCallback funcPtrToCallProxyCallback, FuncPtrToOnNativeProxyCall funcPtrToOnNativeProxyCall);

XPLPC_EXPORT
void xplpc_core_finalize();

XPLPC_EXPORT
bool xplpc_core_is_initialized();

XPLPC_EXPORT
void xplpc_native_call_proxy(char *key, size_t keySize, char *data, size_t dataSize);

XPLPC_EXPORT
void xplpc_native_call_proxy_callback(char *key, size_t keySize, char *data, size_t dataSize);

#endif
