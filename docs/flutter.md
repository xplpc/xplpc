# Flutter

Flutter is a versatile platform that supports a wide range of operating systems, including iOS, Android, macOS, Linux and Windows.

To ensure seamless integration with these diverse environments, developers are required to build XPLPC with interface support that allows for direct C function calls from Dart.

This interface support is essential for ensuring that your Flutter app can take full advantage of the powerful features offered by C functions, such as enhanced performance and optimized memory usage.

With XPLPC and interface support, developers can easily create cross-platform applications that run seamlessly on a range of operating systems and deliver a consistent user experience across all devices.

Whether you're developing for mobile or desktop, Flutter with XPLPC provides a powerful and flexible framework that empowers developers to create robust and performant applications.

# Supported platforms

We currently support these Flutter platforms:

*   Android
*   iOS
*   macOS
*   Linux
*   Windows

*Obs: WebAssembly is currently not tested.*

## How to build the native library for Android

Execute on terminal:

    python3 xplpc.py kotlin-build --interface --platform flutter
    python3 xplpc.py kotlin-build-aar --platform flutter

## How to build the native library for iOS

Execute on terminal:

    python3 xplpc.py swift-build --interface --platform ios-flutter
    python3 xplpc.py swift-build-xcframework --platform ios-flutter

## How to build the native library for macOS

Execute on terminal:

    python3 xplpc.py swift-build --interface --platform macos-flutter
    python3 xplpc.py swift-build-xcframework --platform macos-flutter

## How to build the native library for Linux

Execute on terminal:

    python3 xplpc.py c-build-shared

## How to build the native library for Windows

Execute on terminal:

    python3 xplpc.py c-build-shared

## Setup your project

To setup your project to use the XPLPC plugin for Flutter, keep in mind that you will need add reflectable (https://github.com/google/reflectable.dart) because of Flutter limitation about reflection.

Since Flutter is compiled without reflection support, you can't use the Dart package `dart:mirrors` for reflection.

Follow these steps to setup your project:

1.  Add the reflectable dependencies in your `pubspec.yaml` file:

```yaml
dependencies:
  reflectable: ^4.0.0`

dev_dependencies:
  build_runner: ^2.0.0
```

2.  Add your Dart file that contains the `main` method in the `build.yaml` file, like this: `flutter/plugin/example/build.yaml`. Create or copy the `build.yaml` file to your project root.
3.  Run `build_runner` to generate `reflectable` files ignored for git with this command in your project root folder: `flutter pub run build_runner build --delete-conflicting-outputs`.
4.  Initialize reflectable library in your `main` method like this:

```dart
import 'main.reflectable.dart'; // this file is auto generated in step 3

void main() {
  initializeReflectable();
}
```

5.  Anotate every class that you will use with XPLPC with `@reflectable` and create a method `fromJson`, like this class: `flutter/plugin/lib/type/dataview.dart`.

## Syntax sugar

You can use `callAsync` to execute the method as async function, example:

```dart
var request = Request("sample.login", [
    Param("username", "paulo"),
    Param("password", "123456"),
    Param("remember", true),
]);

String? response = await Client.callAsync<String>(request);
println("Returned Value: $response")
```

## Sample project

Before using the sample you need:

1.  Build the `native library` for your platform first.
2.  Run the `build_runner` inside example folder:

        cd flutter/plugin/example
        flutter pub get
        flutter pub run build_runner build --delete-conflicting-outputs

You can see the sample project in directory `flutter/plugin/example`.

To run the sample execute:

    cd flutter/plugin/example
    flutter run

Or you can specify the device/platform:

    cd flutter/plugin/example
    flutter run -d windows

<img width="280" src="https://github.com/xplpc/xplpc/blob/main/extras/images/screenshot-flutter-android.png?raw=true">

<img width="280" src="https://github.com/xplpc/xplpc/blob/main/extras/images/screenshot-flutter-ios.png?raw=true">

<img width="280" src="https://github.com/xplpc/xplpc/blob/main/extras/images/screenshot-flutter-macos.png?raw=true">

<img width="280" src="https://github.com/xplpc/xplpc/blob/main/extras/images/screenshot-flutter-windows.png?raw=true">

<img width="280" src="https://github.com/xplpc/xplpc/blob/main/extras/images/screenshot-flutter-linux.png?raw=true">
