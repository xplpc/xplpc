# Swift

## How to build the library for Swift

Execute on terminal:

    python3 xplpc.py swift-build

or

    cmake -S . -B build/swift/ios-arm64 -GXcode -DXPLPC_TARGET=swift -DXPLPC_ADD_CUSTOM_DATA=ON -DCMAKE_TOOLCHAIN_FILE=cmake/ios.toolchain.cmake -DPLATFORM=OS64 -DDEPLOYMENT_TARGET=11.0
    cmake --build build/swift/ios-arm64 --config Release

## How to build the XCFramework package for iOS, tvOS, watchOS and macCatalyst

Execute on terminal:

    python3 xplpc.py swift-build-xcframework

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

## Sample project

You can see the sample project in directory `swift/sample`.

<img width="280" src="https://github.com/xplpc/xplpc/blob/main/extras/images/screenshot-ios.png?raw=true">
