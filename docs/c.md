# C

XPLPC has been designed to support the C programming language, catering to the needs of low-level platforms. With this integration, any programming language or platform can leverage the power of native proxy methods and bind to them for communication. This capability opens up a world of possibilities for seamless integration and collaboration between different platforms and technologies.

Whether you are working on a small or large project, the support for C can help you achieve your goals by bridging the gap between different systems and platforms. The flexible design of the library ensures that it can be easily adapted to your specific requirements and needs. With the ability to call native proxy methods, you can access a wealth of functionality and resources that would otherwise be difficult to reach.

## Using the ABI

The library exposes eight functions and nothing else, so a host in any language that can call C reaches it through them.

```
void xplpc_core_initialize(bool initializeCxxNativePlatformProxy,
                           FuncPtrToOnInitializePlatform,
                           FuncPtrToOnFinalizePlatform,
                           FuncPtrToOnNativeProxyCall,
                           FuncPtrToOnNativeProxyCallback,
                           FuncPtrToOnNativeProxyCallFromThread,
                           FuncPtrToOnNativeProxyCallbackFromThread);
void xplpc_core_finalize(void);
bool xplpc_core_is_initialized(void);
void xplpc_native_call_proxy(const char *key, size_t keySize, const char *data, size_t dataSize);
void xplpc_native_call_proxy_callback(const char *key, size_t keySize, const char *data, size_t dataSize);
void xplpc_native_add_mapping(const char *name, size_t nameSize);
void xplpc_native_clear_mappings(void);
void xplpc_free(void *ptr);
```

The host declares the names it owns with `xplpc_native_add_mapping`, which is what lets the library route a call without calling back into a runtime the current thread may not be able to enter.

### Nothing raises into your code

The library is written in C++ and a C++ exception unwinding out of one of these functions is
undefined in the C abi, which in practice ends the process. Every entry point therefore catches
before it returns, reports what failed and answers the empty value for its own return type, so a
host reaching the library through these eight functions never has to prepare for anything crossing
back except the values they declare.

That matters for the two ordinary ways to get there rather than for exotic ones, which are a failed
allocation on a device under memory pressure and a mapping of your own that raises.

### The lifecycle

`xplpc_core_initialize` takes the callbacks the host answers through and makes the library able to
serve a call. `xplpc_core_finalize` takes that away again: it answers every call that is still
waiting, drops every callback pointer, clears the names the host declared, and asks the host to
finalize its own platform through the pointer it gave. `xplpc_core_is_initialized` answers whether a
call can be served right now, so it answers false after finalizing and true again after initializing
a second time.

Initializing while the library is already up rebinds the six callbacks and leaves everything else
alone, since the proxies are registered once. That is what lets a runtime with more than one host
per process take the channel over, which is how a second Dart isolate is answered at all, and it is
also why a call still in flight when it happens is answered through whichever host bound last.
Bringing the library up from two places in one host is therefore a mistake this layer cannot detect
for you.

A call still waiting when the host finalizes is answered with the empty value before the callback
pointers are dropped, since nothing that could ever resolve it survives the call to finalize. That
answer travels through the pointer for the thread it is produced on like any other, so a host that
finalizes from a thread it did not receive a call on is answered through
`FuncPtrToOnNativeProxyCallbackFromThread` and owns the buffers it receives.

A call that arrives after finalizing is answered with the empty value rather than reaching anything
that was released, which is what keeps a caller from waiting on a library that is no longer there.

### The four callbacks are two pairs

Two things travel from the library to the host, and each one arrives through one of two pointers. The suffix says which thread it arrived on and nothing else about what it means, so each pair is the same event and belongs in the same handler.

| Event | On a thread already running a call the host started | On any other thread |
| --- | --- | --- |
| The library asks the host to run a mapping it declared | `FuncPtrToOnNativeProxyCall` | `FuncPtrToOnNativeProxyCallFromThread` |
| The library returns the answer to a call the host made | `FuncPtrToOnNativeProxyCallback` | `FuncPtrToOnNativeProxyCallbackFromThread` |

The direction back into the library needs no such pair. The host starts a call with `xplpc_native_call_proxy` and answers a mapping it was asked to run with `xplpc_native_call_proxy_callback`, both from whatever thread it happens to be on, because entering C is always allowed.

### When the FromThread pointers are the ones called

Two situations, and neither is unusual:

* The host called `xplpc_native_call_proxy`, the mapping needed time and answered later from a thread of its own. The answer arrives through `FuncPtrToOnNativeProxyCallbackFromThread`, which is what a network request on the other side produces.
* Native code reached a mapping the host declared, from a thread the host did not start. It arrives through `FuncPtrToOnNativeProxyCallFromThread`.

So a host that only ever makes calls answered inline never sees them, and any host that talks to something taking time does.

### Reading an answer in the same frame

The abi has one way in and the answer always arrives through a callback, so a host that wants the
value where it made the call builds that itself. Register the key, call
`xplpc_native_call_proxy`, and once it returns read whether the answer already came back under that
key. Every bridge in this repository does exactly that and publishes it as `callSync`.

It works because a mapping answering before it returns has already reached your callback by the time
`xplpc_native_call_proxy` returns, through the plain `FuncPtrToOnNativeProxyCallback` and on the
calling thread. A mapping that takes time has not, and the honest thing then is to drop the key and
carry on with the empty value rather than to wait, since blocking the thread an answer may need to
reach you on is a deadlock nothing here can get you out of.

### What a host has to do differently there

Two things, and forgetting either one is a defect that only shows up later.

Release both buffers with `xplpc_free` once they have been read, since a `FromThread` callback owns them. The plain callbacks must not be freed, since the library still owns those.

Hand the work to your own runtime rather than doing it where the callback ran. The callback arrives on a thread the library happened to be on, which for a runtime that can only be entered from one thread is a thread it cannot be entered from at all. A Dart host queues the answer onto its event loop for exactly this reason, and a Python host, which can be entered from anywhere, still needs the owned buffers because the native frame is already gone.

### Passing null

Both `FromThread` pointers are optional, and a host that passes null is saying those two situations cannot happen to it. When one happens anyway the library reports it and answers the caller with the empty value rather than leaving it waiting, so the failure is a missing answer in a log rather than a hang. A host that does anything asynchronous should implement them.

### Buffers

Every buffer crosses as a pointer plus a size and is never null terminated, so reading one with `strlen` reads past the end. A null pointer is refused and reported rather than read.

The plain callbacks borrow their buffers for the length of the call, so a host that keeps anything copies it first. The `FromThread` callbacks hand over ownership, so a host reads them and then releases each one with `xplpc_free`.

### What the buffers carry

The buffers are a json document, and the format is the one the C++ core defines, which every other bridge agrees with byte for byte. A request names a function and carries its parameters:

```
{"f":"sample.login","p":[{"n":"username","v":"paulo"},{"n":"password","v":"123456"},{"n":"remember","v":true}]}
```

A response carries one value:

```
{"r":"LOGGED-WITH-REMEMBER"}
```

A few rules are part of the format rather than of the encoder that happened to write it:

| Value | On the wire |
| --- | --- |
| Whitespace | None, since the bridge pays for every byte it copies |
| No parameters | `"p":[]`, an empty array, never null and never absent |
| Null | `null`, which is both a parameter carrying no value and a mapping with nothing to answer |
| Character | The integer code point |
| Date | Integer milliseconds since the unix epoch |
| DataView | `{"ptr":<address>,"size":<bytes>}` |

Key order is not part of it, since a json object is unordered and one of the bridges does not preserve the order its own type declared.

The empty string is not a document and is never parsed as one. It is the answer every failing path produces, which is what a caller receives when nothing owns the function it named, when the host cannot be reached, or when the library was finalized while the call was still waiting.

### A host in C

Everything above in one program. It brings the library up, declares a mapping of its own, makes a call into C++ and answers a call that comes back:

```c
#include "xplpc/c/platform.h"

#include <stdio.h>
#include <string.h>

static char answer[256];

static void onInitializePlatform(void)
{
    static const char name[] = "platform.battery.level";
    xplpc_native_add_mapping(name, strlen(name));
}

static void onFinalizePlatform(void)
{
}

static void onNativeProxyCall(const char *key, size_t keySize, const char *data, size_t dataSize)
{
    (void)data;
    (void)dataSize;

    static const char response[] = "{\"r\":\"100\"}";
    xplpc_native_call_proxy_callback(key, keySize, response, strlen(response));
}

static void onNativeProxyCallback(const char *key, size_t keySize, const char *data, size_t dataSize)
{
    (void)key;
    (void)keySize;

    size_t size = dataSize < sizeof(answer) - 1 ? dataSize : sizeof(answer) - 1;
    memcpy(answer, data, size);
    answer[size] = '\0';
}

int main(void)
{
    xplpc_core_initialize(true, onInitializePlatform, onFinalizePlatform, onNativeProxyCall, onNativeProxyCallback, NULL, NULL);

    static const char key[] = "HOST-1";
    static const char request[] = "{\"f\":\"sample.login\",\"p\":[{\"n\":\"username\",\"v\":\"paulo\"},{\"n\":\"password\",\"v\":\"123456\"},{\"n\":\"remember\",\"v\":true}]}";

    xplpc_native_call_proxy(key, strlen(key), request, strlen(request));
    printf("%s\n", answer);

    xplpc_core_finalize();
    return 0;
}
```

Both `FromThread` pointers are null here, which says this host never answers off the thread it was called on. That holds because every mapping it reaches answers before returning, and a host that talks to anything taking time implements them instead.

The key is the host's to choose and only has to be unique among the calls it has waiting, since the library hands it back untouched. Reading the answer straight after `xplpc_native_call_proxy` returns is what every bridge publishes as `callSync`, and it only works for a mapping that answered inline.

Build it against the shared library:

```
clang -std=c11 -o host host.c -Ic/lib/include -Lbuild/c-shared/<arch>/lib -lxplpc
```

## Requirements

A host reaching the library through the abi writes plain C and needs nothing beyond a C compiler, since the header is valid C. The library itself is built as C++20.

## How to build the library for C

Execute on terminal:

```
python3 xplpc.py c-build-static
```

or

```
cmake -S . -B build/c-static -DXPLPC_TARGET=c-static -DXPLPC_ADD_CUSTOM_DATA=ON
cmake --build build/c-static
```

Obs: A shared version can also be built by simply replacing "c-static" with "c-shared".

## How to build the C sample

Execute on terminal:

```
python3 xplpc.py c-build-sample
```

or

```
cmake -S . -B build/c-sample -DXPLPC_TARGET=c-static -DXPLPC_BUILD_SAMPLE=ON
cmake --build build/c-sample
./build/c-sample/bin/xplpc
```

## How to build the C sample to check leaks

Execute on terminal:

```
python3 xplpc.py c-build-leaks
```

or

```
cmake -S . -B build/c-leaks -DXPLPC_TARGET=c-static -DXPLPC_BUILD_SAMPLE=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build/c-leaks
MallocStackLogging=1 leaks --atExit --list -- ./build/c-leaks/bin/xplpc
```

## How to run the C sample

Execute on terminal:

```
python3 xplpc.py c-run-sample
```

## How to run the C tests

Execute on terminal:

```
python3 xplpc.py c-test
```

The suite also builds under instrumentation:

```
python3 xplpc.py c-test --sanitizer address
python3 xplpc.py c-test --sanitizer thread
python3 xplpc.py c-test --sanitizer undefined
```

## How to format the C code

Execute on terminal:

```
python3 xplpc.py c-format
```

## Build parameters

You can build this target using some parameters to change configuration:

**Build type:**

```
--build debug
```

**Incremental, keeping what a previous build left:**

```
--incremental
```

**No dependencies:**

```
--no-deps
```

## Logging

The library reports every failing path. An error names what failed and where, and is always reported. A debug line carries the reason behind it, including the message the parser produced, and is off until you ask for it.

The C library carries the same core, so the level is raised the same way:

```
XPLPC_LOG_LEVEL=debug ./build/c-sample/<arch>/c/sample/bin/xplpc-sample
```

The accepted values are the spdlog level names, from `trace` to `off`, and the library writes to a logger named `XPLPC` without touching the default one. A C++ host can take that logger out of the spdlog registry and redirect it, as the C++ guide shows.
