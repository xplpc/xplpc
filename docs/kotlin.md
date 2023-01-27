# Kotlin

## How to build the library for Kotlin

Execute on terminal:

    python3 xplpc.py kotlin-build

or

    cmake -S . -B build/kotlin -DXPLPC_TARGET=kotlin -DXPLPC_ADD_CUSTOM_DATA=ON -DCMAKE_TOOLCHAIN_FILE=${NDK_ROOT}/build/

<!---->

    cmake/android.toolchain.cmake

<!---->

    cmake --build build/kotlin

## How to build the AAR package for Android

Execute on terminal:

    python3 xplpc.py kotlin-build-aar

or

    cd kotlin/lib
    ./gradlew clean :library:build

## How to format the Kotlin code

Execute on terminal:

    python3 xplpc.py kotlin-format

## Sample project

You can see the sample project in directory `kotlin/sample`.

<img width="280" src="https://github.com/xplpc/xplpc/blob/main/extras/images/screenshot-android.png?raw=true">
