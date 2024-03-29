# C++

When use `Client` it will use any `PlatformProxy` implemented for the `target platform` (JNI, ObjC and others) inside `PlatformProxyList`.

It transfer serialized data between platforms using memory.

Because this you need implement method `xplpc::proxy::initializePlatform` to initialize custom things on C++ side.

See the custom data implementation for sample: `cxx/custom/src/xplpc/custom/NativePlatformProxy.cpp`.

If you don't implement this method, you will get this error when compile:

    ld: error: undefined symbol: xplpc::proxy::NativePlatformProxy::initializePlatform()

## How to build the library for C++

Execute on terminal:

    python3 xplpc.py cxx-build-static

or

    cmake -S . -B build/cxx-static -DXPLPC_TARGET=cxx-static -DXPLPC_ADD_CUSTOM_DATA=ON
    cmake --build build/cxx-static

Obs: A shared version can also be built by simply replacing "cxx-static" with "cxx-shared".

## How to build the C++ sample

Execute on terminal:

    python3 xplpc.py cxx-build-sample

or

    cmake -S . -B build/cxx-sample -DXPLPC_TARGET=cxx-static -DXPLPC_BUILD_SAMPLE=ON
    cmake --build build/cxx-sample
    ./build/cxx-sample/bin/xplpc

## How to build the C++ sample to check leaks

Execute on terminal:

    python3 xplpc.py cxx-build-leaks

or

    cmake -S . -B build/cxx-leaks -DXPLPC_TARGET=cxx-static -DXPLPC_BUILD_SAMPLE=ON -DCMAKE_BUILD_TYPE=Debug
    MallocStackLogging=1 cmake --build build/cxx-leaks
    leaks --atExit --list -- ./build/cxx-leaks/bin/xplpc

## How to format the C++ code

Execute on terminal:

    python3 xplpc.py cxx-format

## Build parameters

You can build this target using some parameters to change configuration:

**Build type:**

    --build debug

**Enable C interface:**

    --interface

**Dry run:**

    --dry

**No dependencies:**

    --no-deps

## Syntax sugar

You can use `callAsync` to execute the method as future function, example:

```cxx
auto request = Request{
    "sample.login",
    Param<std::string>{"username", "paulo"},
    Param<std::string>{"password", "123456"},
    Param<bool>{"remember", true},
};

auto future = Client::callAsync<std::string>(request);
auto response = future.get();
std::cout << "Returned Value: " << (response ? response.value() : "Empty") << std::endl;
```
