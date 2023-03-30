# C

XPLPC has been designed to support the C programming language, catering to the needs of low-level platforms. With this integration, any programming language or platform can leverage the power of native proxy methods and bind to them for communication. This capability opens up a world of possibilities for seamless integration and collaboration between different platforms and technologies.

Whether you are working on a small or large project, the support for C can help you achieve your goals by bridging the gap between different systems and platforms. The flexible design of the library ensures that it can be easily adapted to your specific requirements and needs. With the ability to call native proxy methods, you can access a wealth of functionality and resources that would otherwise be difficult to reach.

## How to build the library for C

Execute on terminal:

    python3 xplpc.py c-build-static

or

    cmake -S . -B build/c-static -DXPLPC_TARGET=c-static -DXPLPC_ADD_CUSTOM_DATA=ON
    cmake --build build/c-static

## How to build the C sample

Execute on terminal:

    python3 xplpc.py c-build-sample

or

    cmake -S . -B build/c-sample -DXPLPC_TARGET=c-static -DXPLPC_ENABLE_SAMPLES=ON
    cmake --build build/c-sample
    ./build/c-sample/bin/xplpc

## How to build the C sample to check leaks

Execute on terminal:

    python3 xplpc.py c-build-leaks

or

    cmake -S . -B build/c-leaks -DXPLPC_TARGET=c-static -DXPLPC_ENABLE_SAMPLES=ON -DCMAKE_BUILD_TYPE=Debug
    MallocStackLogging=1 cmake --build build/c-leaks
    leaks --atExit --list -- ./build/c-leaks/bin/xplpc

## How to format the C code

Execute on terminal:

    python3 xplpc.py c-format

## Build parameters

You can build this target using some parameters to change configuration:

**Build type:**

    --build debug

**Dry run:**

    --dry

**No dependencies:**

    --no-deps
