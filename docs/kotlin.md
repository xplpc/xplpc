# Kotlin

Kotlin support on XPLPC allows you to build, run, and test samples for both Android and Desktop platforms.

When executing the commands, simply pass the `--platform android` or `--platform desktop` parameter.

## Requirements for Android

*   Android NDK (if you are using `CPM` as your package manager)

If you are using `CPM` as your package manager, you will need to manually install the `Android NDK` and set the `ANDROID_NDK_ROOT` environment variable to the root folder of the NDK after installation.

To download the NDK, please visit the following link: https://developer.android.com/ndk/downloads

## How to build a Kotlin library for Android

Execute on terminal:

    python3 xplpc.py kotlin-build --platform android

or

    cmake -S . -B build/kotlin-android -DXPLPC_TARGET=kotlin -DXPLPC_ADD_CUSTOM_DATA=ON -DCMAKE_TOOLCHAIN_FILE=${ANDROID_NDK_ROOT}/build/cmake/android.toolchain.cmake
    cmake --build build/kotlin-android

## How to build the AAR package for Android

Execute on terminal:

    python3 xplpc.py kotlin-build-aar --platform android

or

    cd kotlin/android/lib
    ./gradlew clean :library:build

To enable C interface you can use:

    python3 xplpc.py kotlin-build-aar --interface --platform android

## Requirements for Desktop

*   Java 11 or later

## How to build a Kotlin library for Desktop

Execute on terminal:

    python3 xplpc.py kotlin-build --platform desktop

or

    cmake -S . -B build/kotlin-desktop -DXPLPC_TARGET=kotlin -DXPLPC_ADD_CUSTOM_DATA=ON
    cmake --build build/kotlin-desktop

## How to build the JAR package for Desktop

Execute on terminal:

    python3 xplpc.py kotlin-build-jar --platform desktop

or

    cd kotlin/desktop/lib
    ./gradlew clean jar

## How to format the Kotlin code

Execute on terminal:

    python3 xplpc.py kotlin-format

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

**Platform:**

    --platform (android or desktop)

## Sample project

You can see the sample project in directory `kotlin/android/sample` and `kotlin/desktop/sample`.

<img width="280" src="https://github.com/xplpc/xplpc/blob/main/extras/images/screenshot-android.png?raw=true">

<img width="280" src="https://github.com/xplpc/xplpc/blob/main/extras/images/screenshot-android2.png?raw=true">
