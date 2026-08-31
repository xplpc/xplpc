# Swift

## Requirements

* Xcode
* Command Line Tools

Make sure that you have installed both `Xcode` and `Command Line Tools` on your system.

To download `Xcode` from Apple, please visit the following link: https://apps.apple.com/us/app/xcode/id497799835.

Once installed, open the terminal and execute the command `xcode-select --install` to install the `Command Line Tools`.

The framework is built for iOS 12, tvOS 12, watchOS 5, Mac Catalyst 13.1 and macOS 10.15, which are declared in `core/targets` and read from there by both the build and the Swift package.

`callAsync` needs a higher floor than the framework on three of those, since it is `async` and Swift concurrency arrived later: iOS 13, tvOS 13 and watchOS 6. A consumer below one of those uses `call` and its callback, which every version the framework supports carries.

## How to build the library for Swift

Execute on terminal:

```
python3 xplpc.py swift-build --platform=ios
```

or

```
cmake -S . -B build/swift/ios-arm64 -GXcode -DXPLPC_TARGET=swift -DXPLPC_ADD_CUSTOM_DATA=ON -DCMAKE_TOOLCHAIN_FILE=cmake/ios/ios.toolchain.cmake -DPLATFORM=OS64 -DDEPLOYMENT_TARGET=12.0
cmake --build build/swift/ios-arm64 --config Release
```

## How to build the XCFramework package for iOS, tvOS, watchOS and macCatalyst

Execute on terminal:

```
python3 xplpc.py swift-build-xcframework --platform=ios
```

or

```
xcodebuild -create-xcframework \
	-framework build/swift/ios-arm64/lib/${BUILD_TYPE}/xplpc.framework \
	-framework build/swift/ios-sim-arm64/lib/${BUILD_TYPE}/xplpc.framework \
	-framework build/swift/tvos-arm64/lib/${BUILD_TYPE}/xplpc.framework \
	-framework build/swift/tvos-sim-x64/lib/${BUILD_TYPE}/xplpc.framework \
	-framework build/swift/watchos-arm64/lib/${BUILD_TYPE}/xplpc.framework \
	-framework build/swift/watchos-sim-x64/lib/${BUILD_TYPE}/xplpc.framework \
	-framework build/swift/catalyst-arm64/lib/${BUILD_TYPE}/xplpc.framework \
	-output build/xcframework/xplpc.xcframework
```

The Catalyst framework is the one that cannot be built by pointing a build at its directory. `xcodebuild` honours a destination only when it is given a scheme, so a target build discards the `-macabi` triple and writes a plain macOS binary under a Catalyst name, which links and passes and is reported by nothing. Read the platform back with `vtool -show-build` and expect `MACCATALYST`.

## How to publish a Swift Package

A consumer that reaches the library through Swift Package Manager gets a binary target, so the package points at an archive rather than at these sources.

Build every Apple slice into one xcframework, then generate the package:

```
python3 xplpc.py swift-build --platform=apple
python3 xplpc.py swift-build-xcframework --platform=apple
python3 xplpc.py swift-generate-spm-package
```

The url defaults to the release named by the version in `CMakeLists.txt`, which is `https://github.com/xplpc/xplpc/releases/download/v1.0.0/xplpc.xcframework.zip` today. Publishing anywhere else means naming it:

```
python3 xplpc.py swift-generate-spm-package --url=https://example.com/1.0.0/xplpc.xcframework.zip
```

That writes `build/swift-apple-xcframework/xplpc.xcframework.zip` and, beside it, a `Package.swift` carrying its checksum. Both are build output and neither is committed. Publish the archive to the exact url the package names, since Swift Package Manager refuses an archive whose bytes do not match what the package declares, and regenerate both whenever either one changes.

The package covers iOS, iOS simulator, tvOS, tvOS simulator, watchOS, watchOS simulator, Mac Catalyst and macOS. The `apple` platform exists for this and carries all of them at once, which is what a single binary target needs, since building `ios` and `macos` separately produces two xcframeworks and neither one is enough.

The supported platforms are read from the same target definitions the build uses, so raising a deployment target in `core/targets` is carried into the package by regenerating it.

The manifest itself comes from `swift/spm/Package.swift.in`, which is the file to edit when the package has to declare something new.

## How to run the Swift tests

Execute on terminal:

```
python3 xplpc.py swift-test
```

The same suite also runs on an iOS simulator, against the shipped xcframework rather than the sources:

```
python3 xplpc.py swift-build --platform=ios
python3 xplpc.py swift-build-xcframework --platform=ios
python3 xplpc.py swift-test --platform ios
```

And on a connected iPhone, which signs the sample rather than skipping it:

```
python3 xplpc.py swift-test --platform ios-device
```

## How to run the Swift tests under a sanitizer

The tests on macOS are an XCTest bundle the runner opens with `dlopen`, and address and thread both need their runtime in the process before it starts, so only undefined runs there:

```
python3 xplpc.py swift-test --sanitizer undefined
```

Thread is reachable on the simulator instead, because xcodebuild launches the host application and can instrument it. It has no runtime for a device triple, so the library is built from the simulator slices alone:

```
python3 xplpc.py swift-build --interface --platform ios-simulator --sanitizer thread
python3 xplpc.py swift-build-xcframework --platform ios-simulator
python3 xplpc.py swift-test --platform ios --sanitizer thread
```

## How to build the iOS sample

The Xcode project is generated from `project.yml` and regenerated before every build, so it cannot drift from the spec:

```
python3 xplpc.py swift-build-sample
```

Generating it without building is the same step on its own, which is what to run before opening the project in Xcode:

```
python3 xplpc.py swift-generate-sample-project
```

## How to format the Swift code

Execute on terminal:

```
python3 xplpc.py swift-format
```

## Build parameters

You can build this target using some parameters to change configuration:

**Platform:**

```
--platform ios
```

Platforms available: `apple`, `ios`, `ios-simulator`, `macos`, `ios-flutter`, `macos-flutter`, `test`

The `ios-device` platform is accepted by `swift-test` alone, since it names a connected device rather than a set of slices to build.

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

The framework initializes itself when it loads, so there is nothing to call. What is left is registering the mappings this side owns, by the name every other language reaches them through:

```swift
import xplpc

MappingList.shared.add(name: "platform.battery.level", item: MappingItem(target: batteryLevel))
```

Registering reaches the native side, so it fails rather than registering into a bridge that is not there when the library has not been brought up yet.

## Making a call

There are three ways to make a call and they differ in how the answer reaches you.

`call` hands the answer to a callback:

```swift
let request = Request(
    "sample.login",
    Param("username", "paulo"),
    Param("password", "123456"),
    Param("remember", true)
)

Client.call(request) { (response: String?) in
    print(response ?? "")
}
```

`callAsync` hands it to what the language waits on:

```swift
let response: String? = await Client.callAsync(request)
print(response ?? "")
```

Both give control back before the answer arrives, so neither one holds up whoever called you. A `callAsync` awaited from a `@MainActor` context resumes on the main actor, which is what puts the answer back on the interface without any work from you.

The callback given to `call` runs on the thread that produced the answer, which is the mapping's thread when the mapping answered later. Anything it touches has to be safe there.

Touching a view from it is undefined, so anything that reaches the interface is handed to the main queue:

```swift
Client.call(request) { (response: String?) in
    DispatchQueue.main.async {
        self.label.text = response ?? ""
    }
}
```

A mapping that answers right away runs the callback on the calling thread, where the same code without that hop happens to work. Writing the hop anyway is what keeps it working when that mapping later starts answering from a thread of its own.

`callSync` answers with the value itself:

```swift
let response: String? = Client.callSync(request)
print(response ?? "")
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

```swift
Client.call(requestData) { (response: String) in
    print(response)
}

let answer = Client.callSync(requestData)

let awaited = await Client.callAsync(requestData)
```

## Writing a mapping

A mapping is handed a message and something to answer through, and the one rule is to return quickly. Where the work runs is your decision.

Answering right away is fine when there is nothing slow to do:

```swift
func batteryLevel(message: Message, r: @escaping Response) {
    let suffix: String = message.get("suffix") ?? ""

    r("100\(suffix)")
}
```

Anything that takes time is started and left running, and the answer is given when it finishes:

```swift
func ip(message: Message, r: @escaping Response) {
    guard let url = URL(string: "https://httpbin.io/ip") else {
        r(nil)
        return
    }

    URLSession.shared.dataTask(with: url) { data, _, _ in
        r(AnyCodable(String(data: data ?? Data(), encoding: .utf8) ?? ""))
    }.resume()
}
```

The mapping runs on whichever thread the call arrived on. Doing slow work there instead of handing it to a queue holds that thread for as long as the work takes, which is what the rule above is about.

## Sending a large buffer

`DataView` carries an address and a size across the bridge, so a large buffer crosses without being copied or encoded. Swift storage has no fixed address, so the address is only valid inside the closure that produced it and the view must not escape it:

```swift
var bytes: [UInt8] = readSomething()

DataView.withByteArray(&bytes) { view in
    let request = Request("sample.image.grayscale.dataview", Param("dataView", view))

    Client.call(request) { (response: String?) in
        print(response ?? "")
    }
}
```

Reading one goes through `ByteArrayHelper`, which answers an empty array for a view that carries no address rather than reading from it:

```swift
let bytes = ByteArrayHelper.createFromDataView(view)
```

Two rules follow from the view carrying no ownership. A mapping that answers with a view has to keep the buffer alive after it returns, since the bridges read it once the call has unwound. And a view is released only by the side that allocated it, so a view handed to a mapping is never freed by that mapping.

## Logging

The library reports every failing path. An error names what failed and where, and is always reported. A debug line carries the reason behind it, including the message the parser produced, and is off until you ask for it.

Everything is written to the `XPLPC` subsystem through `os_log`, so errors show up with no setup and the debug level is enabled per subsystem when you want it:

```
sudo log config --subsystem XPLPC --mode "level:debug"
```

Then read it with:

```
log stream --predicate 'subsystem == "XPLPC"' --level debug
```

In Xcode, the same messages appear in the console, and Console.app shows them once "Include Debug Messages" is on.

## Sample project

You can see the sample project in directory `swift/sample`.

<img width="280" src="https://github.com/xplpc/xplpc/blob/main/extras/images/screenshot-ios.png?raw=true">

<img width="280" src="https://github.com/xplpc/xplpc/blob/main/extras/images/screenshot-ios2.png?raw=true">
