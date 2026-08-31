# C++

When use `Client` it will use any `PlatformProxy` implemented for the `target platform` (JNI, ObjC and others) inside `PlatformProxyList`.

It transfer serialized data between platforms using memory.

Because this you need implement method `xplpc::proxy::initializePlatform` to initialize custom things on C++ side.

See the custom data implementation for sample: `cxx/custom/src/xplpc/custom/NativePlatformProxy.cpp`.

If you don't implement this method, you will get this error when compile:

```
ld: error: undefined symbol: xplpc::proxy::NativePlatformProxy::initializePlatform()
```

## Requirements

The library is built as C++20, which is what `co_await` on the awaitable `callAsync` answers with needs. A consumer compiling against these headers builds as C++20 as well.

## How to build the library for C++

Execute on terminal:

```
python3 xplpc.py cxx-build-static
```

or

```
cmake -S . -B build/cxx-static -DXPLPC_TARGET=cxx-static
cmake --build build/cxx-static
```

`XPLPC_ADD_CUSTOM_DATA` is off by default and belongs to this repository's own sample, which defines
`initializePlatform` and the `sample.*` mappings the suites call. Turning it on gives you those and
leaves you no place to define your own, so a consumer leaves it off.

That matters beyond the static library, because every shared build has to carry a definition of
`initializePlatform` and there is no linker to ask for one. The aar, the xcframework, the wasm module
and the shared library the Python package ships are all built with this repository's sample compiled
in, which is about a third of the object code. To put your own mappings there instead, point the
build at your own directory:

```
cmake -S . -B build/c-shared \
  -DXPLPC_TARGET=c-shared \
  -DXPLPC_ENABLE_INTERFACE=ON \
  -DXPLPC_ADD_CUSTOM_DATA=ON \
  -DXPLPC_CUSTOM_DATA_PATH=/path/to/your/data \
  -DXPLPC_CUSTOM_DATA_INCLUDE_PATH=/path/to/your/data/include
```

That directory carries a `CMakeLists.txt` appending your sources to `XPLPC_SRC_FILES`, and one of
them defines `initializePlatform`. `cxx/custom` is the worked example.

Everything a consumer reaches is behind one header:

```cpp
#include "xplpc/xplpc.hpp"
```

## How to build the C++ sample

Execute on terminal:

```
python3 xplpc.py cxx-build-sample
```

or

```
cmake -S . -B build/cxx-sample -DXPLPC_TARGET=cxx-static -DXPLPC_BUILD_SAMPLE=ON
cmake --build build/cxx-sample
./build/cxx-sample/bin/xplpc
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
MallocStackLogging=1 leaks --atExit --list -- ./build/cxx-leaks/bin/xplpc
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

A C++ host builds the native proxy, initializes it, registers it, and then brings the library up:

```cxx
auto proxy = std::make_shared<NativePlatformProxy>();
proxy->initialize();

PlatformProxyList::shared()->append(proxy);

XPLPC::initialize();
```

`XPLPC::initialize` is the last step rather than the first, and it says the library can serve a call. Anything a call needs has to be in place before it, which is why the proxy is built, given its mappings and registered above it. Nothing is lost by leaving it out except the ability to make a call, since every call is refused and reported until it has run.

`initialize` calls `initializePlatform`, which is declared by the library and defined by you, and is where the mappings this side owns are registered:

```cxx
namespace xplpc
{
namespace proxy
{

void NativePlatformProxy::initializePlatform()
{
    MappingList::shared()->add("platform.battery.level", Map::create<std::string, std::string>({"suffix"}, &batteryLevel));
}

} // namespace proxy
} // namespace xplpc
```

The list given to `Map::create` names the parameters, in the order the types after the return type declare them, which is how a value on the wire is decoded against the type the signature asked for. `batteryLevel` is written in the section below, and it is the same mapping the examples in this guide call.

## Making a call

There are three ways to make a call and they differ in how the answer reaches you.

`call` hands the answer to a callback:

```cxx
auto request = Request{
    "platform.battery.level",
    Param<std::string>{"suffix", "%"},
};

Client::call<std::string>(request, [](const auto &response) {
    std::cout << (response ? response.value() : "") << std::endl;
});
```

`callAsync` is awaited from a coroutine:

```cxx
auto response = co_await Client::callAsync<std::string>(request);
std::cout << (response ? response.value() : "") << std::endl;
```

Both give control back before the answer arrives, so neither one holds up whoever called you. C++ has no scheduler to return you anywhere, so a coroutine resumes on the thread that produced the answer, and the callback given to `call` runs there too. Anything either of them touches has to be safe on that thread, and anything captured has to be owned, since the calling frame is long gone.

`callSync` answers with the value itself:

```cxx
auto response = Client::callSync<std::string>(request);
std::cout << (response ? response.value() : "") << std::endl;
```

It reads an answer that is already there rather than waiting for one, so it works only when the mapping answers before it returns. A mapping that takes time is reported and answered with the empty value, and the registration it would have resolved is dropped rather than left behind, so nothing is queued and nothing is leaked. It never blocks a thread, which is why there is no timeout to pass and no deadlock to reach.

Whether a mapping answers before it returns belongs to the mapping rather than to the call site, and that mapping may be written in another language by somebody else. A call that works today answers empty the day that mapping starts doing real work, so `callSync` is for reading something you know is a plain read.

The empty value is also what a mapping with nothing to answer produces, so the two are told apart by the log and not by the value.

## Making the same call from a document

Every entry point has a second form that takes the request already serialized and answers the
response document rather than a decoded value. It is what serves a host that received a document
from somewhere else and has nothing to decode it against, which is how the bridges themselves call
in.

The difference in the answer is the whole of it. A typed call says a value is absent the way this
language says it, and a call from a document answers the empty string, since a document is either
there or it is not.

```cpp
Client::call(requestData, [](const std::string &response) {
    std::cout << response << std::endl;
});

auto answer = Client::callSync(requestData);

auto awaited = co_await Client::callAsync(requestData);
```

## Writing a mapping

A mapping is handed a message and something to answer through, and the one rule is to return quickly. Where the work runs is your decision.

Answering right away is fine when there is nothing slow to do:

```cxx
void batteryLevel(const Message &m, const Response &r)
{
    r(std::string{"100"} + m.get<std::string>("suffix").value_or(""));
}
```

Anything that takes time is started and left running, and the answer is given when it finishes:

```cxx
void query(const Message &m, const Response &r)
{
    auto sql = m.get<std::string>("sql").value_or("");

    std::thread([r, sql] {
        r(runQuery(sql));
    }).detach();
}
```

The responder is captured by value, since the answer is given after this frame is gone. The mapping runs on whichever thread the call arrived on, so doing slow work there instead of handing it to a thread holds that thread for as long as the work takes.

## Sending a large buffer

`DataView` carries an address and a size across the bridge, so a large buffer crosses without being copied or encoded. The buffer is yours and the view only borrows it, which means it has to stay where it is and stay alive for the whole call:

```cxx
std::vector<uint8_t> bytes = readSomething();

auto view = DataView{bytes.data(), bytes.size()};

auto request = Request{
    "sample.image.grayscale.dataview",
    Param{"dataView", view},
};
```

Reading one is the same borrow in the other direction. `ptr()` and `size()` describe the bytes and `copy` writes them somewhere you own:

```cxx
std::vector<uint8_t> target(view.size());
view.copy(target.data());
```

Two rules follow from the view carrying no ownership. A mapping that answers with a view has to keep the buffer alive after it returns, since the bridges read it once the call has unwound, so a local is not enough. And a view decoded from an answer can carry no address at all, which is what every failing path produces, so `copy` writes nothing rather than reading from it.

## Logging

The library reports every failing path. An error names what failed and where, and is always reported. A debug line carries the reason behind it, including the message the parser produced, and is off until you ask for it.

Raise the level through the environment, without rebuilding:

```
XPLPC_LOG_LEVEL=debug ./build/cxx-sample/<arch>/cxx/sample/bin/xplpc-sample
```

The accepted values are the spdlog level names, from `trace` to `off`, and anything else is reported and leaves only errors on.

The library writes to a spdlog logger named `XPLPC` and never touches the default logger or the global level, so an application that uses spdlog for its own logging keeps the setup it chose.

`XPLPC::initialize` publishes that logger, so you can send it wherever you want without affecting your own:

```cpp
XPLPC::initialize();

auto logger = Log::logger();
logger->sinks().clear();
logger->sinks().push_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>("xplpc.log", true));
```

It is in the spdlog registry under the same name, so `spdlog::get("XPLPC")` answers the same logger and is what a host reaching this library through another language would use.
