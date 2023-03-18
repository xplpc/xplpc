#ifndef XPLPC_C_TYPEDEFS_H
#define XPLPC_C_TYPEDEFS_H

#include <stddef.h>

// function: void()
typedef void (*FuncPtrToOnInitializePlatform)();

// function: void()
typedef void (*FuncPtrToOnFinalizePlatform)();

// function: bool(name, name size)
typedef bool (*FuncPtrToOnHasMapping)(char *, size_t);

// function: void(key, key size, data, data size)
typedef void (*FuncPtrToOnNativeProxyCallback)(char *, size_t, char *, size_t);

// function: void(key, key size, data, data size)
typedef void (*FuncPtrToOnNativeProxyCall)(char *, size_t, char *, size_t);

#endif
