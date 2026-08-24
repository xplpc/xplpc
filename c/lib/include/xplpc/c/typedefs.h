#ifndef XPLPC_C_TYPEDEFS_H
#define XPLPC_C_TYPEDEFS_H

#include <stdbool.h>
#include <stddef.h>

typedef void (*FuncPtrToOnInitializePlatform)(void);

typedef void (*FuncPtrToOnFinalizePlatform)(void);

typedef void (*FuncPtrToOnNativeProxyCall)(const char *, size_t, const char *, size_t);

typedef void (*FuncPtrToOnNativeProxyCallback)(const char *, size_t, const char *, size_t);

// The buffers belong to the callee and are released with xplpc_free.
typedef void (*FuncPtrToOnHostBufferOwner)(char *, size_t, char *, size_t);

// Both channels hand the host buffers it owns, so they are the same shape.
typedef FuncPtrToOnHostBufferOwner FuncPtrToOnNativeProxyCallFromThread;

typedef FuncPtrToOnHostBufferOwner FuncPtrToOnNativeProxyCallbackFromThread;

#endif
