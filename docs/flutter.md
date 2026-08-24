# Flutter

Flutter is a versatile platform that supports a wide range of operating systems, including iOS, Android, macOS, Linux and Windows.

To ensure seamless integration with these diverse environments, developers are required to build XPLPC with interface support that allows for direct C function calls from Dart.

This interface support is essential for ensuring that your Flutter app can take full advantage of the powerful features offered by C functions, such as enhanced performance and optimized memory usage.

With XPLPC and interface support, developers can easily create cross-platform applications that run seamlessly on a range of operating systems and deliver a consistent user experience across all devices.

Whether you're developing for mobile or desktop, Flutter with XPLPC provides a powerful and flexible framework that empowers developers to create robust and performant applications.

## Supported platforms

We currently support these Flutter platforms:

* Android
* iOS
* macOS
* Linux
* Windows

*Obs: WebAssembly is currently not tested.*

## How to build the native library for Android

Execute on terminal:

```
python3 xplpc.py kotlin-build --interface --platform flutter
python3 xplpc.py kotlin-build-aar --platform flutter
```

## How to build the native library for iOS

Execute on terminal:

```
python3 xplpc.py swift-build --interface --platform ios-flutter
python3 xplpc.py swift-build-xcframework --platform ios-flutter
```

## How to build the native library for macOS

Execute on terminal:

```
python3 xplpc.py swift-build --interface --platform macos-flutter
python3 xplpc.py swift-build-xcframework --platform macos-flutter
```

## How to build the native library for Linux

Execute on terminal:

```
python3 xplpc.py c-build-shared
```

## How to build the native library for Windows

Execute on terminal:

```
python3 xplpc.py c-build-shared
```

## Setup your project

To setup your project to use the XPLPC plugin for Flutter, keep in mind that you will need add reflectable (https://github.com/google/reflectable.dart) because of Flutter limitation about reflection.

Since Flutter is compiled without reflection support, you can't use the Dart package `dart:mirrors` for reflection.

Follow these steps to setup your project:

1. Add the reflectable dependencies in your `pubspec.yaml` file:

```yaml
dependencies:
  reflectable: ^5.2.3

dev_dependencies:
  build_runner: ^2.15.1
  reflectable_builder: ^1.2.3
```

2. Add your Dart file that contains the `main` method in the `build.yaml` file, like this: `flutter/plugin/example/build.yaml`. Create or copy the `build.yaml` file to your project root.
3. Run `build_runner` to generate `reflectable` files ignored for git with this command in your project root folder: `dart run build_runner build`.
4. Initialize reflectable library in your `main` method like this:

```dart
import 'main.reflectable.dart'; // this file is auto generated in step 3

void main() {
  initializeReflectable();
}
```

5. Anotate every class that you will use with XPLPC with `@reflectable` and create a method `fromJson`, like this class: `flutter/plugin/lib/type/dataview.dart`.

## How to run the Flutter tests

The suite drives the real native library, so it has to be built first:

```
python3 xplpc.py swift-build --interface --platform macos-flutter
python3 xplpc.py swift-build-xcframework --platform macos-flutter
python3 xplpc.py flutter-test
```

## How to format the Dart code

Execute on terminal:

```
python3 xplpc.py flutter-format
```

## Bringing the library up

Reflectable is initialized first, as the setup above describes, and then the library is given the serializer it carries:

```dart
void main() {
  initializeReflectable();

  XPLPC.instance.initialize(Config(JsonSerializer()));

  Mapping.instance.initialize();

  runApp(const MyApp());
}
```

A mapping is registered by name, which is what makes it reachable from any other language:

```dart
MappingList.instance.add(
  "platform.battery.level",
  MappingItem(batteryLevel),
);
```

Registering reaches the native side, so it fails rather than registering into a bridge that is not there when the library has not been brought up yet.

The native library is found by the platform it runs on, which is the application bundle on iOS and macOS, `libxplpc.so` on Android and Linux and `xplpc.dll` on Windows. `XPLPC_LIBRARY_PATH` names a file to open instead, which is what serves a desktop application that ships the library somewhere the loader does not search.

## One isolate owns the bridge

The library is loaded once per process and the callbacks it answers through are created with
`Pointer.fromFunction`, which the Dart runtime binds to the isolate that made them. That is what
lets a call be answered before it returns, and it is why the bridge belongs to one isolate.

Initializing from a second isolate takes those callbacks over. A call the first isolate is still
waiting on is then answered into the second, and the runtime does not raise for that, it ends the
process with `Cannot invoke native callback from a different isolate`.

So bring the library up on one isolate and call it from there. Work you hand to `compute` or to an
`Isolate.spawn` sends its request back to that isolate rather than reaching the library itself.

## Making a call

There are three ways to make a call and they differ in how the answer reaches you.

`call` hands the answer to a callback:

```dart
var request = Request("sample.login", [
    Param("username", "paulo"),
    Param("password", "123456"),
    Param("remember", true),
]);

Client.call<String>(request, (response) {
    print(response);
});
```

`callAsync` hands it to what the language waits on:

```dart
final response = await Client.callAsync<String>(request);
print(response);
```

Both give control back before the answer arrives, so neither one holds up whoever called you.

An answer produced on a thread the isolate does not own reaches you through a `NativeCallable.listener`, which queues it onto the event loop. That is part of the plugin and needs nothing from you.

`callSync` answers with the value itself, and `callSyncFromString` takes the encoded request:

```dart
final response = Client.callSync<String>(request);
print(response);
```

It reads an answer that is already there rather than waiting for one, so it works only when the mapping answers before it returns. A mapping that takes time is reported and answered with the empty value, and the registration it would have resolved is dropped rather than left behind, so nothing is queued and nothing is leaked. It never blocks a thread, which is why there is no timeout to pass and no deadlock to reach.

Whether a mapping answers before it returns belongs to the mapping rather than to the call site, and that mapping may be written in another language by somebody else. A call that works today answers empty the day that mapping starts doing real work, so `callSync` is for reading something you know is a plain read.

The empty value is also what a mapping with nothing to answer produces, so the two are told apart by the log and not by the value.

## Writing a mapping

A mapping is handed a message and something to answer through, and the one rule is to return quickly. Where the work runs is your decision.

Answering right away is fine when there is nothing slow to do:

```dart
void batteryLevel(Message m, Response r) {
  r("100${m.get<String>("suffix") ?? ""}");
}
```

Anything that takes time is started and left running, and the answer is given when it finishes:

```dart
void ip(Message m, Response r) {
  http.get(Uri.parse("https://httpbin.io/ip")).then((response) => r(response.body));
}
```

An isolate runs on one thread, so a mapping that waits instead of returning holds that thread and everything on it. Starting the work and returning is what keeps the interface responsive.

## Sending a large buffer

`DataView` carries an address and a size across the bridge, so a large buffer crosses without being copied or encoded. Which of the two constructors you use decides who releases the memory. `createFromByteBuffer` borrows native memory you own, and `allocateFromByteArray` owns what it allocated and releases it with `dispose`:

```dart
final view = DataView.allocateFromByteArray(bytes);

try {
  final request = Request("sample.image.grayscale.dataview", [Param("dataView", view)]);

  Client.call<String>(request, (response) {
    print(response ?? "");
  });
} finally {
  view.dispose();
}
```

Reading one goes through `ByteArrayHelper`, which answers an empty list for a view that carries no address rather than reading from it, since reading address zero takes the isolate down:

```dart
final bytes = ByteArrayHelper.createFromDataView(view);
```

A view is released only by the side that allocated it, so a view handed to a mapping is never disposed by that mapping, and a mapping that answers with a view has to keep its buffer alive after it returns.

## Logging

The library reports every failing path. An error names what failed and where, and is always reported. A debug line carries the reason behind it, including the message the parser produced, and is off until you ask for it.

The library writes to a `package:logging` logger named `XPLPC`, and your application decides the level and where it goes:

```dart
import 'package:logging/logging.dart';

hierarchicalLoggingEnabled = true;

Logger('XPLPC').level = Level.ALL;

Logger.root.onRecord.listen((record) {
  debugPrint('${record.loggerName}: ${record.level.name}: ${record.message}');
});
```

Setting the level on a named logger needs `hierarchicalLoggingEnabled`, which is what lets you raise this library without raising your own. Without a listener nothing is printed, which is how `package:logging` works for every logger.

## Sample project

Before using the sample you need:

1. Build the `native library` for your platform first.
2. Run the `build_runner` inside example folder:

   ```
   cd flutter/plugin/example
   flutter pub get
   dart run build_runner build
   ```

You can see the sample project in directory `flutter/plugin/example`.

To run the sample execute:

```
cd flutter/plugin/example
flutter run
```

Or you can specify the device/platform:

```
cd flutter/plugin/example
flutter run -d windows
```

<img width="280" src="https://github.com/xplpc/xplpc/blob/main/extras/images/screenshot-flutter-android.png?raw=true">

<img width="280" src="https://github.com/xplpc/xplpc/blob/main/extras/images/screenshot-flutter-ios.png?raw=true">

<img width="280" src="https://github.com/xplpc/xplpc/blob/main/extras/images/screenshot-flutter-macos.png?raw=true">

<img width="280" src="https://github.com/xplpc/xplpc/blob/main/extras/images/screenshot-flutter-windows.png?raw=true">

<img width="280" src="https://github.com/xplpc/xplpc/blob/main/extras/images/screenshot-flutter-linux.png?raw=true">
