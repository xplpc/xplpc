# C++

The router that carries every call lives in the C++ core. Each language attaches to it as a layer, and a call from any of them reaches the layer that registered the function name last, with the request and the answer carried as serialized data through memory.

The C++ mappings are registered in `xplpc::layer::NativeLayer::initializePlatform`, which the library declares and you define. `cxx/custom/src/xplpc/custom/NativeLayer.cpp` is the definition the suites and the libraries the host languages load are built with, while the C++ and C samples each define their own. A build that links the library without one fails with:

```
ld: error: undefined symbol: xplpc::layer::NativeLayer::initializePlatform()
```

## Requirements

The library is built as C++20, which is what `co_await` on the awaitable `callAsync` answers with needs. A consumer compiling against these headers builds as C++20 as well.

## Using the library from your own CMake project

The library is a CMake project you add to yours, so you keep your own repository and take updates by
moving one tag rather than by copying this one. `FetchContent` and `add_subdirectory` both work, and
`cxx/consumer` is a complete project built this way that a job builds and runs on every push.

```cmake
include(FetchContent)

FetchContent_Declare(
    xplpc
    GIT_REPOSITORY https://github.com/xplpc/xplpc.git
    GIT_TAG main-<commit>
    GIT_SHALLOW TRUE
)

set(XPLPC_TARGET "cxx-static" CACHE STRING "What the library is built for")
FetchContent_MakeAvailable(xplpc)

target_sources(xplpc PRIVATE src/NativeLayer.cpp src/MyMappings.cpp)
target_include_directories(xplpc PRIVATE include)

add_executable(app src/main.cpp)
target_link_libraries(app PRIVATE xplpc::xplpc)
```

Name the release you build against in `GIT_TAG`. Every release is named `main-` followed by the
commit it was cut from, and the version the manifests declare does not change between them. To build
against a local checkout instead, configure with `-DFETCHCONTENT_SOURCE_DIR_XPLPC=/path/to/xplpc`, which is what the job
does with the checkout it is testing.

Your mappings go into the `xplpc` target itself, through `target_sources`, and one of those files
defines `initializePlatform`. That is the one form that serves every target: a shared library, the
aar, the framework and the wasm module have no executable to carry the definition, so it has to be
inside the library. A static build would also accept it from your executable, and the same lines
work there too.

`XPLPC_TARGET` picks what the library is built as, which is one of `cxx-static`, `c-static`,
`c-shared`, `kotlin`, `swift` and `wasm`, and it defaults to `cxx-static`. The rest of the options
are off unless you turn them on, except `XPLPC_ENABLE_SERIALIZER_FOR_JSON`, which is on and stays
on since json is the only serializer, and configuring as a subproject leaves your build alone: it forces
no build type, sets no output directories and builds no tests or samples. Linking `xplpc::xplpc`
gives your code C++20, the serializer definition, `SPDLOG_NO_TLS` and the include directories of
spdlog and nlohmann, which the headers need.

Everything a consumer reaches is behind one header:

```cpp
#include "xplpc/xplpc.hpp"
```

The examples in this guide are written as if `xplpc::client`, `xplpc::message`, `xplpc::type` and `xplpc::util` were brought in with `using namespace`.

## How to build the library for C++

Execute on terminal:

```
python3 xplpc.py cxx-build-static
```

or

```
cmake -S . -B build/cxx-static -DXPLPC_TARGET=cxx-static -DXPLPC_ADD_CUSTOM_DATA=ON
cmake --build build/cxx-static
```

`XPLPC_ADD_CUSTOM_DATA` is off by default and builds this repository's own sample into the library,
which defines `initializePlatform` and the `sample.*` mappings the suites call. The tasks that build
the static and shared libraries, the test suites and the kotlin, swift and wasm libraries turn it on,
since those artifacts have to carry a definition. To build one of them with your mappings
instead, point the build at a directory whose `CMakeLists.txt` adds them to the `xplpc` target the
way `cxx/custom` does:

```
cmake -S . -B build/c-shared \
  -DXPLPC_TARGET=c-shared \
  -DXPLPC_ADD_CUSTOM_DATA=ON \
  -DXPLPC_CUSTOM_DATA_PATH=/path/to/your/mappings
```

Every task that builds the native library takes the same directory through `--custom-data-path`,
which is `c-build-static`, `c-build-shared`, `cxx-build-static`, `kotlin-build`, `swift-build` and
`wasm-build`, and so do the c++, c and macOS swift suites, which build it for themselves. The aar, jar and xcframework tasks package what that build produced, so that is how
they and the shared library the Python and Flutter packages load carry your mappings instead of this
repository's. A directory that holds no `CMakeLists.txt` is refused before any build is touched:

```
python3 xplpc.py kotlin-build --platform android --custom-data-path=/path/to/your/mappings
```

## How to run the consumer project

Execute on terminal:

```
python3 xplpc.py cxx-test-consumer
```

It configures `cxx/consumer` against this checkout through `FetchContent`, builds it and runs it,
and the run fails unless the consumer's own mapping answered.

## How to build the C++ sample

Execute on terminal:

```
python3 xplpc.py cxx-build-sample
```

or

```
cmake -S . -B build/cxx-sample -DXPLPC_TARGET=cxx-static -DXPLPC_BUILD_SAMPLE=ON
cmake --build build/cxx-sample
./build/cxx-sample/cxx/sample/bin/xplpc-sample
```

## How to run the C++ sample

Execute on terminal:

```
python3 xplpc.py cxx-run-sample
```

## How to build the C++ sample to check leaks

Execute on terminal:

```
python3 xplpc.py cxx-build-leaks
```

or

```
cmake -S . -B build/cxx-leaks -DXPLPC_TARGET=cxx-static -DXPLPC_BUILD_SAMPLE=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build/cxx-leaks
MallocStackLogging=1 leaks --atExit --list -- ./build/cxx-leaks/cxx/sample/bin/xplpc-sample
```

## How to run the C++ tests

Execute on terminal:

```
python3 xplpc.py cxx-test
```

The suite also builds under instrumentation, which is where a race or a memory defect actually shows up:

```
python3 xplpc.py cxx-test --sanitizer address
python3 xplpc.py cxx-test --sanitizer thread
python3 xplpc.py cxx-test --sanitizer undefined
```

Leak detection comes with the address sanitizer on Linux only, macOS aborts when asked for it.

## How to format the C++ code

Execute on terminal:

```
python3 xplpc.py cxx-format
```

## Build parameters

You can build this target using some parameters to change configuration:

**Build type:**

```
--build debug
```

**Enable C interface:**

```
--interface
```

**Incremental, keeping what a previous build left:**

```
--incremental
```

**No dependencies:**

```
--no-deps
```

## Bringing the library up

There is nothing to call. The router is built the first time anything uses the library and lives until the process ends. The first call, or the first host attaching, runs `initializePlatform` once, and that is where the C++ mappings are registered:

```cpp
namespace xplpc
{
namespace layer
{

void NativeLayer::initializePlatform()
{
    data::MappingList::shared()->add("platform.battery.level", map::Map::create<std::string, std::string>({"suffix"}, &batteryLevel));
}

} // namespace layer
} // namespace xplpc
```

It runs inside whichever call or attach came first, on that thread, so it registers mappings and does nothing else. Calling the library from inside it reaches the one time step that is still running it.

The list given to `Map::create` names the parameters, in the order the types after the return type declare them, which is how a value on the wire is decoded against the type the signature asked for. `batteryLevel` is written in the section on mappings below, and it is the mapping the examples in this guide call.

## The last registration wins

Registering a name makes the layer that registered it the owner of that name. Every language registers into the same router, so when two of them register the same name, the one that registered it last receives every call to it. Taking a registration back gives the name to the layer that registered it before.

The C++ mappings are registered before any host attaches, which is what makes this useful. C++ can serve `http.request`, an Android application can register the same name and take it over, and a Flutter layer in that application can take it from both. When the Flutter layer goes away the name goes back to Android, and when Android takes its registration back the name goes back to C++.

`MappingList::shared()->clear()` takes back every name C++ registered. Registering a name again after a host took it makes C++ its owner again, since that is now the last registration.

## Making a call

There are three ways to make a call and they differ in how the answer reaches you.

`call` hands the answer to a callback:

```cpp
auto request = Request{
    "platform.battery.level",
    Param<std::string>{"suffix", "%"},
};

Client::call<std::string>(request, [](const auto &response) {
    std::cout << (response ? response.value() : "") << std::endl;
});
```

`callAsync` is awaited from a coroutine:

```cpp
auto response = co_await Client::callAsync<std::string>(request);
std::cout << (response ? response.value() : "") << std::endl;
```

A call is awaited once. The awaitable can be moved and not copied, awaiting one held in a variable takes `co_await std::move(awaitable)`, and awaiting it a second time raises `std::logic_error` at the `co_await` rather than answering before the call did.

Destroying a coroutine while it waits cancels the call, so an answer that arrives afterwards resumes nothing and leaves nothing behind. That holds when the destruction happens before the answer, and the coroutine is yours to destroy only while nothing else can resume it. An answer that arrived first has already begun resuming the coroutine on the thread that produced it, and destroying it at that moment is the same as destroying a coroutine while it runs, which no awaitable can make safe. Destroy a waiting coroutine on the thread or executor that delivers its answers, or stop the work that would answer it first.

On gcc, prefer a task type whose `initial_suspend` suspends. The thread sanitizer reports a race on gcc 15 between the frame written by the code that starts an eager coroutine and the end of its body on the thread that resumed it, which neither side of `callAsync` controls. A task that starts suspended finishes starting before its body runs, so the two never overlap.

`callAsync` suspends only when the answer is not already there, and continues at once when the mapping answered inline. `call` returns before the answer whenever the mapping needs time, and runs the callback before it returns when the mapping answered at once. C++ has no scheduler to return you anywhere, so a coroutine resumes on the thread that produced the answer, and the callback given to `call` runs there too. Anything either of them touches has to be safe on that thread, and anything captured has to be owned, since the calling frame is long gone.

`callSync` answers with the value itself:

```cpp
auto answered = Client::callSync<std::string>(request);
std::cout << (answered ? answered.value() : "") << std::endl;
```

It reads an answer that is already there rather than waiting for one, so it works only when the mapping answers before it returns. A call the mapping had not answered by then is cancelled and reported, so its later answer resolves nothing, and `callSync` answers the empty value. It never blocks a thread, which is why there is no timeout to pass and no deadlock to reach.

Whether a mapping answers before it returns belongs to the mapping rather than to the call site, and that mapping may be written in another language by somebody else. A call that works today answers empty the day that mapping starts doing real work, so `callSync` is for reading something you know is a plain read.

The empty value is also what a mapping with nothing to answer produces, so the two are told apart by the log and not by the value.

## Making the same call from a document

Every entry point has a second form that takes the request already serialized and answers the response document rather than a decoded value. It is what serves a host that received a document from somewhere else and has nothing to decode it against.

The difference in the answer is the whole of it. A typed call says a value is absent the way this language says it, and a call from a document answers the empty string, since a document is either there or it is not.

```cpp
const auto requestData = std::string{R"({"f":"platform.battery.level","p":[{"n":"suffix","v":"%"}]})"};

Client::call(requestData, [](const std::string &document) {
    std::cout << document << std::endl;
});

auto answer = Client::callSync(requestData);

auto awaited = co_await Client::callAsync(requestData);
```

## When no mapping answers

Every call is answered exactly once, and a call no mapping can answer is answered with the empty value rather than left waiting:

| Situation | What the caller receives |
| --- | --- |
| No layer registered the name | The empty value at once, and the failure is reported |
| The request could not be read | The empty value at once, and the failure is reported |
| The mapping raised | The empty value, and the failure is reported |
| The layer serving the call detached | The empty value, and the name goes back to the layer that registered it before |
| The call was cancelled | Nothing through a callback, since a cancelled call never calls back, and `callSync` answers the empty value |

A C++ call is cancelled by destroying the coroutine that awaits it and by `callSync` when the mapping had not answered. The C++ layer never detaches, so it is the layer a name falls back to when every host that registered it is gone.

## Writing a mapping

A mapping is handed a message and something to answer through, and the one rule is to return quickly. Where the work runs is your decision.

Answering right away is fine when there is nothing slow to do:

```cpp
void batteryLevel(const Message &m, const Response &r)
{
    r(std::string{"100"} + m.get<std::string>("suffix").value_or(""));
}
```

Anything that takes time is started and left running, and the answer is given when it finishes:

```cpp
void query(const Message &m, const Response &r)
{
    auto sql = m.get<std::string>("sql").value_or("");

    std::thread([r, sql] {
        try
        {
            r(runQuery(sql));
        }
        catch (...)
        {
            r(nullptr);
        }
    }).detach();
}
```

A value is read only as a type that holds it, at the top or inside a standard sequence, set, map, array, pair or tuple, and an enumeration only as the integer under it or as the text its own serializer maps. A boolean is never a number, a fraction never reads as an integer, an integer reads as a floating type only when that type holds it exactly, a magnitude past a floating type is refused rather than read as infinity, and a timestamp past what the declared clock can count is refused rather than overflowing. A mapping with nothing to answer calls `r(nullptr)`, which reaches the caller as null whatever type the mapping declared. The [types guide](types.md) has every type and what each language reads it as.

The responder is captured by value, since the answer is given after this frame is gone. The mapping runs on whichever thread the call arrived on, so doing slow work there instead of handing it to a thread holds that thread for as long as the work takes.

## Sending a large buffer

A `DataView` names a buffer by a handle, so a large buffer crosses without being copied or encoded, and the wire carries `{"buffer":<handle>}` and never an address. The bytes are registered by whoever owns them, and in C++ that is a `Buffer`, which registers them for as long as it lives and releases them when it is destroyed:

```cpp
std::vector<uint8_t> bytes = readSomething();
Buffer buffer{bytes.data(), bytes.size()};

auto imageRequest = Request{
    "sample.image.grayscale.dataview",
    Param<DataView>{"dataView", buffer.view()},
};
```

The bytes stay yours, so they have to stay valid and in place for as long as the `Buffer` lives, and the `Buffer` has to outlive the call, since a mapping that answers later reads them after `call` returned. A cancel ends the call without stopping the mapping, which includes the one `callSync` makes when the mapping had not answered and the one a destroyed awaitable makes, so after either of those the bytes are kept until the mapping is done with them.

Reading a view asks for the bytes it names:

```cpp
const auto view = buffer.view();

if (const auto span = view.bytes())
{
    std::vector<uint8_t> copy(span->data, span->data + span->size);
}
```

`bytes()` answers the owner's bytes in place rather than a copy, which is what lets a mapping write its answer into them, the way `sample.image.grayscale.dataview` does. It answers nothing for a view whose buffer was released, was never registered or carries a handle that names nothing any more, so a stale view is refused and reported rather than read.

Two rules follow from the owner being the only one who releases. A mapping that answers with a view keeps its `Buffer` alive after it returns, since the caller reads the bytes once the call has unwound, so a local is not enough. And a view you received is never released by you, since only the layer that registered a buffer can release it. A layer that detaches releases every buffer it still holds.

## Logging

The library reports every failing path. An error names what failed and where, and is always reported. A debug line carries the reason behind it, including the message the parser produced, and is off until you ask for it.

Raise the level through the environment, without rebuilding:

```
XPLPC_LOG_LEVEL=debug ./build/cxx-sample/cxx/sample/bin/xplpc-sample
```

The accepted values are the spdlog level names, from `trace` to `off`, and anything else is reported and leaves only errors on.

The library writes to a spdlog logger named `XPLPC` and never touches the default logger or the global level, so an application that uses spdlog for its own logging keeps the setup it chose.

The logger is published the first time the library is used, and asking for it publishes it if nothing has used the library yet, so you can send it wherever you want before the first call and without affecting your own:

```cpp
#include "spdlog/sinks/basic_file_sink.h"

auto logger = Log::logger();
logger->sinks().clear();
logger->sinks().push_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>("xplpc.log", true));
```

It is in the spdlog registry under the same name, so `spdlog::get("XPLPC")` answers the same logger and is what a host reaching this library through another language would use. A host that registered a logger under that name before the library came up keeps its own, with the sinks and level it chose, and the library reports through a logger of its own that is not in the registry.
