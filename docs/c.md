# C

The C abi is how a host written in any language that can call C reaches the library. Python and Dart reach it this way, and a host of your own does the same: it attaches a layer to the router, declares the names it serves, makes calls and answers the ones that reach it.

## Using the ABI

The library exposes these functions and nothing else:

```
xplpc_handle xplpc_attach(const xplpc_host *host);
void xplpc_detach(xplpc_handle layer);
void xplpc_abandon(const xplpc_handle *layer);
void xplpc_declare(xplpc_handle layer, const char *name, size_t nameSize);
void xplpc_retract(xplpc_handle layer, const char *name, size_t nameSize);
bool xplpc_call(xplpc_handle layer, uint64_t token, const char *request, size_t requestSize, xplpc_call_result *result);
bool xplpc_cancel(xplpc_handle layer, xplpc_handle call);
void xplpc_answer(xplpc_handle call, const char *response, size_t responseSize);
xplpc_handle xplpc_buffer_register(xplpc_handle layer, void *data, size_t size);
bool xplpc_buffer_read(xplpc_handle buffer, void **data, size_t *size);
bool xplpc_buffer_release(xplpc_handle layer, xplpc_handle buffer);
size_t xplpc_pending(void);
void xplpc_free(void *ptr);
```

A layer, a call and a buffer are each an `xplpc_handle`, which is never zero when it is valid. The library allocates every one of them, and a handle that names something that is gone is refused rather than resolving something newer. A layer handle is accepted only when `xplpc_attach` handed it out, so any other handle, the handle of the C++ layer included, is refused and reported by every function that takes one, except `xplpc_abandon`, which does nothing for a layer that is not attached.

### Nothing raises into your code

The library is written in C++ and a C++ exception unwinding out of one of these functions is undefined in the C abi, which in practice ends the process. Every entry point therefore catches before it returns, reports what failed and answers the empty value for its own return type, so a host never has to prepare for anything crossing back except the values these functions declare.

That matters for the two ordinary ways to get there rather than for exotic ones, which are a failed allocation on a device under memory pressure and a mapping of your own that raises.

### Attaching a host

A host brings the library up by attaching, and there is nothing else to initialize. The router is built the first time anything uses the library and lives until the process ends, and the first attach runs the C++ `initializePlatform` once, so the C++ mappings are registered before any host. `xplpc_attach` answers the handle of the new layer, or zero when the host was refused, in which case the host still owns its context and `post` never carries anything to it.

The host is described by `xplpc_host`:

| Field | What it is |
| --- | --- |
| `context` | Handed back untouched to both callbacks |
| `invoke` | Serves a call synchronously, with the request borrowed for the length of the callback |
| `post` | Carries everything the host receives later, in the order it happened |
| `enterableAnywhere` | Whether `invoke` may be entered on any thread |
| `name`, `nameSize` | An optional name the host attaches under |

`invoke` is entered only on a thread already running a call this host started, or on any thread when the host says it can be entered anywhere. Everything else reaches the host through `post`, which is what lets a runtime that can only be entered from its own thread, such as a Dart isolate, receive a call that arrived on a thread it does not own.

A host attaching under a name another host is attached under takes its place, and the host it replaces is never reached again. The calls the replaced host made are dropped rather than answered, since the callbacks that would receive them belong to a runtime that is gone, which is what a Dart isolate restarting in the same process looks like. A host that passes no name replaces nobody. Taking a place waits for anything still inside the host being replaced, the same way `xplpc_abandon` does, so a host never attaches under its own name from inside one of its own callbacks.

`xplpc_detach` takes the layer away. The router stops routing to it and from it, answers every call it was serving and every call it made with the empty value, gives every name it declared back to the layer that declared it before, and releases every buffer it registered. The answers to the calls it made reach it through `post` like any other answer. Detaching never waits, so it is safe from inside the host's own callbacks. The layer itself is released once nothing is running inside it any more, and the last thing `post` ever carries is `XPLPC_POST_RELEASE`, after which the host may free its context.

`xplpc_abandon` is detaching for a host that is already gone, which is what a runtime calls as it shuts down when every callback it registered dies with it. It does everything detaching does except reach the host: the calls it made are dropped rather than answered, nothing is ever posted to it again, not even the release, and it returns only once no post or invoke that started earlier is still inside the host, so a runtime can delete its callbacks right afterwards. The handle arrives through a pointer because a finalizer hands its callback exactly one, which is how the Dart plugin calls it, and the host keeps that pointer valid for the call. It is never called from inside one of the host's own callbacks, since it waits for those to return, and abandoning a layer that is no longer attached does nothing.

### Declaring names

`xplpc_declare` makes this layer the owner of a name, and `xplpc_retract` gives the name back to the layer that declared it before. The last registration of a name wins, whichever language made it.

The C++ mappings are registered before any host attaches, so C++ can serve `http.request`, an Android layer can declare the same name and take it over, and a Flutter layer in that application can take it from both. When the Flutter layer detaches the name goes back to Android, and when Android retracts it the name goes back to C++. Two hosts in one process follow the same rule, in the order they declared.

### Making a call

`xplpc_call` routes a request and answers true when the call was answered before it returned, with the answer in `result->data` and `result->dataSize`. That buffer belongs to the host, which releases it with `xplpc_free`.

When the mapping needs more time `xplpc_call` answers false and `result->call` holds the handle of the call. The answer arrives later through `post` as `XPLPC_POST_ANSWER`, carrying the token the host passed to `xplpc_call`. The token is the host's to choose and only has to tell its own waiting calls apart, since the library hands it back untouched.

`xplpc_cancel` takes back a waiting call the layer made and answers true when it did, which guarantees its answer never arrives. False means the answer won the race and is on its way, so the host still receives it. A call another layer made is refused, so a host serving a call cannot take it back from its caller. Reading the answer of `xplpc_call` and cancelling when there is none is what every language built on this abi publishes as `callSync`.

A call that no mapping can answer is answered with the empty value rather than left waiting:

| Situation | What the caller receives |
| --- | --- |
| No layer declared the name | The empty value at once, and the failure is reported |
| The request could not be read | The empty value at once, and the failure is reported |
| The mapping raised | The empty value, and the failure is reported |
| The layer serving the call detached | The empty value, and the name goes back to the layer that declared it before |
| The call was cancelled | Nothing, since a cancelled call never answers |

`xplpc_pending` answers how many calls are still waiting anywhere in the process, which is what lets a host prove it left none behind.

### Serving a call

A call to a name the host declared reaches it through `invoke`, or through `post` as `XPLPC_POST_INVOKE` when the host cannot be entered on the thread it arrived on. Either way it carries the handle of the call, and the host answers it once with `xplpc_answer`, at once or later and from any thread.

Answering a call that was cancelled, already answered or dropped by a detach does nothing, so a host never has to know whether its caller is still there.

### What post carries

`post` receives a kind, an identifier and a buffer:

| Kind | Identifier | Buffer |
| --- | --- | --- |
| `XPLPC_POST_ANSWER` | The token the host passed to `xplpc_call` | The response |
| `XPLPC_POST_INVOKE` | The handle of the call to answer | The request |
| `XPLPC_POST_RELEASE` | Zero | None |

The host owns every buffer `post` hands it and releases it with `xplpc_free`, which accepts null. The request `invoke` receives is borrowed instead, so a host that keeps anything from it copies it first.

`post` arrives on whichever thread produced what it carries, so a host that can only be entered from one thread hands the work to its own runtime there rather than doing it inside the callback.

### Strings

Every string crosses as a pointer plus a size and is never null terminated, so reading one with `strlen` reads past the end. A null pointer is refused and reported rather than read.

### What the strings carry

The strings are a json document, and the format is the one the C++ core defines, which every other language agrees with byte for byte. A request names a function and carries its parameters:

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
| DataView | `{"buffer":<handle>}` |

Key order is not part of it, since a json object is unordered and one of the languages does not preserve the order its own type declared.

The empty string is not a document and is never parsed as one. It is the answer every failing path produces, which is what a caller receives when nothing declared the function it named, when the mapping raised, or when the layer serving it detached.

### Buffers

A data view names a buffer by a handle, so a large buffer crosses without being copied or encoded and no address ever crosses the wire. `xplpc_buffer_register` registers bytes the host owns and answers the handle, and the host keeps the bytes valid and in place until it releases them with `xplpc_buffer_release`. The table refuses a handle once it is released and cannot recall an address a reader already holds, so the host keeps the bytes until every mapping it handed the view to is done with them, which `xplpc_cancel` does not end.

`xplpc_buffer_read` answers the address and the size of the bytes a handle names, in place rather than copied, and answers false with a null address and a zero size, reporting the refusal, when the handle names nothing, which is a buffer that was released, was never registered or whose handle was reused. The address stays valid only while the owner keeps the buffer registered, so a host reading a buffer it does not own copies the bytes before handing control back to anything that could release it.

Only the layer that registered a buffer can release it, so releasing one another layer registered answers false. A layer that detaches releases every buffer it still holds, and a mapping that answers with a view keeps its buffer registered after it returns, since the caller reads it once the call has unwound.

### A host in C

Everything above in one program. It attaches, declares a mapping of its own, makes a call into C++ and answers a call that comes back:

```c
#include "xplpc/c/platform.h"

#include <stdio.h>
#include <string.h>

static xplpc_handle layer;
static int released;

static void serve(xplpc_handle call)
{
    static const char response[] = "{\"r\":\"from-host\"}";
    xplpc_answer(call, response, strlen(response));
}

static void onInvoke(void *context, xplpc_handle call, const char *request, size_t requestSize)
{
    (void)context;
    (void)request;
    (void)requestSize;

    serve(call);
}

static void onPost(void *context, xplpc_post_kind kind, uint64_t id, char *data, size_t dataSize)
{
    (void)context;
    (void)dataSize;

    if (kind == XPLPC_POST_INVOKE)
    {
        serve(id);
    }

    if (kind == XPLPC_POST_RELEASE)
    {
        released = 1;
    }

    xplpc_free(data);
}

static int call(const char *request, char *answer, size_t answerSize)
{
    xplpc_call_result result;

    if (!xplpc_call(layer, 1, request, strlen(request), &result))
    {
        xplpc_cancel(layer, result.call);
        return 0;
    }

    size_t size = result.dataSize < answerSize - 1 ? result.dataSize : answerSize - 1;
    memcpy(answer, result.data, size);
    answer[size] = '\0';

    xplpc_free(result.data);
    return 1;
}

int main(void)
{
    const xplpc_host host = {NULL, onInvoke, onPost, false, NULL, 0};
    layer = xplpc_attach(&host);

    if (layer == 0)
    {
        return 1;
    }

    static const char name[] = "platform.reverse.response";
    xplpc_declare(layer, name, strlen(name));

    char login[256];
    char reverse[256];

    if (!call("{\"f\":\"sample.login\",\"p\":[{\"n\":\"username\",\"v\":\"paulo\"},{\"n\":\"password\",\"v\":\"123456\"},{\"n\":\"remember\",\"v\":true}]}", login, sizeof(login)))
    {
        return 1;
    }

    if (!call("{\"f\":\"sample.reverse\",\"p\":[]}", reverse, sizeof(reverse)) || strcmp(reverse, "{\"r\":\"response-is-from-host\"}") != 0)
    {
        return 1;
    }

    printf("%s\n", login);

    xplpc_detach(layer);
    return released ? 0 : 1;
}
```

The host says it cannot be entered anywhere, so `onInvoke` is entered only on a thread running a call this host made, and everything else would reach `onPost`. `sample.reverse` is a C++ mapping that calls `platform.reverse.response` back while it serves the call, on the same thread, so that call reaches `onInvoke`, and the answer the host gives is what `sample.reverse` builds its own answer from.

Every mapping this host reaches answers before it returns, so `call` reads the answer where it made the call and cancels a call that was not answered, which is the shape of `callSync`. A host that talks to anything taking time reads its answers from `XPLPC_POST_ANSWER` instead. Detaching releases the layer, and `onPost` receives `XPLPC_POST_RELEASE` before `xplpc_detach` returns here, since nothing else holds it.

Build it against the shared library, with an rpath naming the directory that holds it so `./host` finds the library when it starts:

```
clang -std=c11 -o host host.c -Ic/lib/include -Lbuild/c-shared/<arch>/lib -lxplpc -Wl,-rpath,"$PWD/build/c-shared/<arch>/lib"
./host
```

The `<arch>` directory is the one `c-build-shared` wrote, which is `arm64` or `x86_64` for the machine that built it.

## Requirements

A host reaching the library through the abi writes plain C and needs nothing beyond a C compiler, since the header is valid C. The library itself is built as C++20. The library a host links carries a definition of the C++ `initializePlatform`, even one that registers nothing, and the [C++ guide](cxx.md) describes how to build it with mappings of your own.

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

A shared version is built with `python3 xplpc.py c-build-shared`, or with `cmake -S . -B build/c-shared/<arch> -DXPLPC_TARGET=c-shared -DXPLPC_ADD_CUSTOM_DATA=ON` and `cmake --build build/c-shared/<arch>`, which is the directory the host program above and the Python and Flutter tasks read.

## How to build the C sample

Execute on terminal:

```
python3 xplpc.py c-build-sample
```

or

```
cmake -S . -B build/c-sample -DXPLPC_TARGET=c-static -DXPLPC_BUILD_SAMPLE=ON
cmake --build build/c-sample
./build/c-sample/c/sample/bin/xplpc-sample
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
MallocStackLogging=1 leaks --atExit --list -- ./build/c-leaks/c/sample/bin/xplpc-sample
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

The C sources are formatted with the rest of the C family, so execute on terminal:

```
python3 xplpc.py cxx-format
```

## Build parameters

You can build this target using some parameters to change configuration:

**Your own C++ mappings in place of `cxx/custom`, as the C++ guide describes:**

```
--custom-data-path=/path/to/your/mappings
```

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
XPLPC_LOG_LEVEL=debug ./build/c-sample/c/sample/bin/xplpc-sample
```

The accepted values are the spdlog level names, from `trace` to `off`, and the library writes to a logger named `XPLPC` without touching the default one. A C++ host can take that logger out of the spdlog registry and redirect it, as the C++ guide shows.
