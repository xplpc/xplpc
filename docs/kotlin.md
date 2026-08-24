# Kotlin

Kotlin support on XPLPC allows you to build, run, and test samples for both Android and Desktop platforms.

When executing the commands, simply pass the `--platform android` or `--platform desktop` parameter.

Disable `wildcard` imports in your IDE (Preferences > Editor >Code Style > Kotlin > Imports):

* Top-Level Symbols: Use single name import
* Java Static and Enum Members: Use single name import
* Packages to Use Import with '\*': Delete all rows

## Requirements for Android

* Java 17 or later
* Android NDK (if you are using `CPM` as your package manager)

If you are using `CPM` as your package manager, you will need to manually install the `Android NDK` and set the `ANDROID_NDK_ROOT` environment variable to the root folder of the NDK after installation.

To download the NDK, please visit the following link: https://developer.android.com/ndk/downloads

## How to build a Kotlin library for Android

Execute on terminal:

```
python3 xplpc.py kotlin-build --platform android
```

or

```
cmake -S . -B build/kotlin-android -DXPLPC_TARGET=kotlin -DXPLPC_ADD_CUSTOM_DATA=ON -DCMAKE_TOOLCHAIN_FILE=${ANDROID_NDK_ROOT}/build/cmake/android.toolchain.cmake
cmake --build build/kotlin-android
```

## How to build the AAR package for Android

Execute on terminal:

```
python3 xplpc.py kotlin-build-aar --platform android
```

or

```
cd kotlin/android/lib
./gradlew clean :library:build
```

To enable C interface you can use:

```
python3 xplpc.py kotlin-build-aar --interface --platform android
```

## Requirements for Desktop

* Java 17 or later

## How to build a Kotlin library for Desktop

Execute on terminal:

```
python3 xplpc.py kotlin-build --platform desktop
```

or

```
cmake -S . -B build/kotlin-desktop -DXPLPC_TARGET=kotlin -DXPLPC_ADD_CUSTOM_DATA=ON
cmake --build build/kotlin-desktop
```

## How to build the JAR package for Desktop

Execute on terminal:

```
python3 xplpc.py kotlin-build-jar --platform desktop
```

or

```
cd kotlin/desktop/lib
./gradlew clean jar
```

## How to build and run the Kotlin sample

Execute on terminal:

```
python3 xplpc.py kotlin-build-sample --platform desktop
python3 xplpc.py kotlin-run-sample --platform desktop
```

## How to run the Kotlin tests

Execute on terminal:

```
python3 xplpc.py kotlin-test --platform desktop
```

This task does not build the native library, so a change under `cxx`, `c` or `jni` is only picked up after building it first:

```
python3 xplpc.py kotlin-build --platform desktop
```

The Android tests run on a device or emulator:

```
python3 xplpc.py kotlin-test --platform android
```

Both read the same sources, in `kotlin/shared/tests`, so a test cannot exist on one platform and not the other. What stays per project is `TestPlatform`, which brings XPLPC up, and `ContextTest`, which means nothing off a device.

## How to format the Kotlin code

Execute on terminal:

```
python3 xplpc.py kotlin-format
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

**Platform:**

```
--platform (android or desktop)
```

## Release builds on Android

The serializer builds the wire format by reading field names through reflection, which is what lets any type cross the bridge without an annotation or a generated adapter. R8 renames fields in a release build, so a type of yours that crosses the bridge stops being readable and the call silently carries the wrong parameters while every debug build keeps working.

The library ships `consumer-rules.pro` and it keeps what belongs to the library, which is the classes JNI resolves by name and the fields the serializer reads to build the wire format. It cannot keep your types, so any class you send or receive needs a rule of your own:

```proguard
-keepclassmembers class com.yourapp.model.** { <fields>; }
```

Build a release variant and make one call carrying such a type before shipping, since this is the one failure that never appears in a debug build.

## Bringing the library up

On Android the library initializes itself, through a content provider the manifest of the AAR already declares, so there is nothing to call. The debug level comes from the application's own debuggable flag.

On desktop nothing runs on its own, so the initializer is called before the first call:

```kotlin
import com.xplpc.platform.PlatformInitializer

PlatformInitializer.initialize()
```

A mapping is registered by name, which is what makes it reachable from any other language:

```kotlin
import com.xplpc.data.MappingList
import com.xplpc.map.MappingItem

MappingList.add("platform.battery.level", MappingItem(::batteryLevel))
```

Registering reaches the native side, so it fails rather than registering into a bridge that is not there when the library has not been brought up yet.

## Making a call

There are three ways to make a call and they differ in how the answer reaches you.

`call` hands the answer to a callback:

```kotlin
val request = Request(
    "sample.login",
    Param("username", "paulo"),
    Param("password", "123456"),
    Param("remember", true)
)

Client.call<String>(request) { response ->
    println(response)
}
```

`callAsync` hands it to what the language waits on:

```kotlin
val response: String? = Client.callAsync<String>(request)
println(response)
```

Both give control back before the answer arrives, so neither one holds up whoever called you. A `callAsync` awaited from a coroutine on `Dispatchers.Main` resumes on that dispatcher, which is what puts the answer back on the interface without any work from you.

The callback given to `call` runs on the thread that produced the answer, which is the mapping's thread when the mapping answered later. Anything it touches has to be safe there.

Touching a view from it throws `CalledFromWrongThreadException`, so anything that reaches the interface is handed to the main thread:

```kotlin
Client.call<String>(request) { response ->
    runOnUiThread { textView.text = response }
}
```

A mapping that answers right away runs the callback on the calling thread, where the same code without that hop happens to work. Writing the hop anyway is what keeps it working when that mapping later starts answering from a thread of its own.

`callSync` answers with the value itself:

```kotlin
val response: String? = Client.callSync<String>(request)
println(response)
```

It reads an answer that is already there rather than waiting for one, so it works only when the mapping answers before it returns. A mapping that takes time is reported and answered with the empty value, and the registration it would have resolved is dropped rather than left behind, so nothing is queued and nothing is leaked. It never blocks a thread, which is why there is no timeout to pass and no deadlock to reach.

Whether a mapping answers before it returns belongs to the mapping rather than to the call site, and that mapping may be written in another language by somebody else. A call that works today answers empty the day that mapping starts doing real work, so `callSync` is for reading something you know is a plain read.

The empty value is also what a mapping with nothing to answer produces, so the two are told apart by the log and not by the value.

## Writing a mapping

A mapping is handed a message and something to answer through, and the one rule is to return quickly. Where the work runs is your decision.

Answering right away is fine when there is nothing slow to do:

```kotlin
fun batteryLevel(data: Message, r: Response) {
    r("100" + (data.get<String>("suffix") ?: ""))
}
```

Anything that takes time goes to a dispatcher of your own, and the answer is given from there:

```kotlin
fun ip(data: Message, r: Response) {
    CoroutineScope(Dispatchers.IO).launch {
        r(URL("https://httpbin.io/ip").readText())
    }
}
```

The mapping runs on whichever thread the call arrived on. Doing slow work there instead of handing it to a dispatcher holds that thread for as long as the work takes, which is what the rule above is about.

## Sending a large buffer

`DataView` carries an address and a size across the bridge, so a large buffer crosses without being copied or encoded. A managed heap moves its objects, so the address is never taken from a `ByteArray` directly. `createFromByteBuffer` borrows a direct buffer you own, and `createFromByteArray` copies into a direct buffer the view keeps alive:

```kotlin
val view = DataView.createFromByteBuffer(buffer)

val request = Request("sample.image.grayscale.dataview", Param("dataView", view))

Client.call<String>(request) { response ->
    println(response ?: "")
}
```

Reading one goes through `ByteArrayHelper`, which answers an empty array for a view that carries no address rather than reading from it:

```kotlin
val bytes = ByteArrayHelper.createFromDataView(view)
```

A buffer passed to `createFromByteBuffer` has to stay alive and direct for the whole call, and a mapping that answers with a view has to keep its buffer alive after it returns, since the bridges read it once the call has unwound.

## Logging

The library reports every failing path. An error names what failed and where, and is always reported. A debug line carries the reason behind it, including the message the parser produced, and is off until you ask for it.

On Android the Kotlin side follows your application's own debuggable flag, so a debug build carries it and a release build reports only errors, with nothing for you to set.

The native side is a separate library and an Android application cannot put a variable in its own environment, so it reads a system property instead:

```
adb shell setprop debug.xplpc.level debug
```

The property is read once when the logger is built, so the application has to be restarted after setting it.

On desktop it follows `XPLPC_LOG_LEVEL`, which is the same variable the native side reads, so one setting raises the whole stack:

```
XPLPC_LOG_LEVEL=debug java -jar your-app.jar
```

Desktop logging goes through `java.util.logging`, whose console handler accepts nothing below `INFO`, so a debug line needs a handler that takes it:

```kotlin
val handler = ConsoleHandler()
handler.level = Level.FINE
Logger.getLogger("XPLPC").addHandler(handler)
```

Everything is written under the `XPLPC` name, which is the logcat tag on Android and the logger name on desktop, so you can filter or silence the library without touching your own logging.

## Sample project

You can see the sample project in directory `kotlin/android/sample` and `kotlin/desktop/sample`.

<img width="280" src="https://github.com/xplpc/xplpc/blob/main/extras/images/screenshot-android.png?raw=true">

<img width="280" src="https://github.com/xplpc/xplpc/blob/main/extras/images/screenshot-android2.png?raw=true">

#### Compose Multiplatform

<img width="280" src="https://github.com/xplpc/xplpc/blob/main/extras/images/screenshot-kotlin-cmp-macos.png?raw=true">

<img width="280" src="https://github.com/xplpc/xplpc/blob/main/extras/images/screenshot-kotlin-cmp-windows.png?raw=true">

<img width="280" src="https://github.com/xplpc/xplpc/blob/main/extras/images/screenshot-kotlin-cmp-linux.png?raw=true">
