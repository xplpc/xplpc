# Swift

## Requirements

*   Xcode
*   Command Line Tools

Make sure that you have installed both `Xcode` and `Command Line Tools` on your system.

To download `Xcode` from Apple, please visit the following link: https://apps.apple.com/us/app/xcode/id497799835.

Once installed, open the terminal and execute the command `xcode-select --install` to install the `Command Line Tools`.

## How to build the library for Swift

Execute on terminal:

    python3 xplpc.py swift-build --platform=ios

or

    cmake -S . -B build/swift/ios-arm64 -GXcode -DXPLPC_TARGET=swift -DXPLPC_ADD_CUSTOM_DATA=ON -DCMAKE_TOOLCHAIN_FILE=cmake/ios/ios.toolchain.cmake -DPLATFORM=OS64 -DDEPLOYMENT_TARGET=12.0
    cmake --build build/swift/ios-arm64 --config Release

## How to build the XCFramework package for iOS, tvOS, watchOS and macCatalyst

Execute on terminal:

    python3 xplpc.py swift-build-xcframework --platform=ios

or

    xcodebuild -create-xcframework \
    	-framework build/swift/ios-arm64/lib/${BUILD_TYPE}/xplpc.framework \
    	-framework build/swift/ios-sim-arm64/lib/${BUILD_TYPE}/xplpc.framework \
    	-framework build/swift/tvos-arm64/lib/${BUILD_TYPE}/xplpc.framework \
    	-framework build/swift/tvos-sim-x64/lib/${BUILD_TYPE}/xplpc.framework \
    	-framework build/swift/watchos-arm64/lib/${BUILD_TYPE}/xplpc.framework \
    	-framework build/swift/watchos-sim-x64/lib/${BUILD_TYPE}/xplpc.framework \
    	-framework build/swift/catalyst-arm64/lib/${BUILD_TYPE}/xplpc.framework \
    	-output build/xcframework/xplpc.xcframework

## How to format the Swift code

Execute on terminal:

    python3 xplpc.py swift-format

## Build parameters

You can build this target using some parameters to change configuration:

**Platform:**

    --platform ios

Platforms available: `ios`, `macos`, `ios-flutter`, `macos-flutter`, `test`

**Build type:**

    --build debug

**Enable C interface:**

    --interface

**Dry run:**

    --dry

**No dependencies:**

    --no-deps

## Syntax sugar

You can use `callAsync` to execute the method as async function, example:

    let request = Request(
        "sample.login",
        Param("username", "paulo"),
        Param("password", "123456"),
        Param("remember", true)
    )

    if let response: String? = await Client.callAsync(request) {
        print("Returned Value: \(response)")
    }

## Sample project

You can see the sample project in directory `swift/sample`.

<img width="280" src="https://github.com/xplpc/xplpc/blob/main/extras/images/screenshot-ios.png?raw=true">

<img width="280" src="https://github.com/xplpc/xplpc/blob/main/extras/images/screenshot-ios2.png?raw=true">
