#pragma once

#include <cstddef>

// function: void(key, key size, data, data size)
typedef void (*FuncPtrToCallProxyCallback)(char *, size_t, char *, size_t);

// function: void(key, key size, data, data size)
typedef void (*FuncPtrToOnNativeProxyCall)(char *, size_t, char *, size_t);
