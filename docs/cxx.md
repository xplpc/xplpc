# C++

When use `RemoteClient` it will use any `PlatformProxy` implemented for the `target platform` (JNI, ObjC and others).

It transfer serialized data between platforms using memory.

Because this you need implement method `xplpc::proxy::initializePlatform` to initialize custom things on C++ side.

See the custom data implementation for samples: `cxx/custom/src/xplpc/custom/PlatformProxy.cpp`.

If you don't implement this method, you will get this error when compile:

    ld: error: undefined symbol: xplpc::proxy::PlatformProxy::initializePlatform()

## How to build the library for C++

Execute on terminal:

    python3 xplpc.py cxx-build

or

    cmake -S . -B build/cxx
    cmake --build build/cxx

## How to build the C++ sample

Execute on terminal:

    python3 xplpc.py cxx-build-sample

or

    cmake -S . -B build/cxx-sample -DXPLPC_TARGET=sample -DXPLPC_ADD_CUSTOM_DATA=ON

<!---->

    cmake --build build/cxx-sample

<!---->

    ./build/cxx-sample/bin/xplpc

## How to build the C++ sample to check leaks

Execute on terminal:

    python3 xplpc.py cxx-build-leaks

or

    cmake -S . -B build/cxx-leaks -DXPLPC_TARGET=sample -DXPLPC_ADD_CUSTOM_DATA=ON -DCMAKE_BUILD_TYPE=Debug

<!---->

    MallocStackLogging=1 cmake --build build/cxx-leaks

<!---->

    leaks --atExit --list -- ./build/cxx-leaks/bin/xplpc

## How to format the C++ code

Execute on terminal:

    python3 xplpc.py cxx-format
