#ifndef XPLPC_C_TYPEDEFS_H
#define XPLPC_C_TYPEDEFS_H

#include <stddef.h>

// function: void(key, key size, data, data size)
typedef void (*FuncPtrToCallProxyCallback)(char *, size_t, char *, size_t);

// function: void(key, key size, data, data size)
typedef void (*FuncPtrToOnNativeProxyCall)(char *, size_t, char *, size_t);

#endif
