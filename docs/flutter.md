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

## How to build the native library for iOS

Execute on terminal:

    python3 xplpc.py flutter-build-xcframework

## How to build the native library for macOS

Execute on terminal:

    python3 xplpc.py flutter-build-xcframework-macos

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

## Sample project

Before using the sample, don't forget to build the native library for your platform first.

You can see the sample project in directory `flutter/plugin/example`.

<img width="280" src="https://github.com/xplpc/xplpc/blob/main/extras/images/screenshot-flutter-android.png?raw=true">

<img width="280" src="https://github.com/xplpc/xplpc/blob/main/extras/images/screenshot-flutter-ios.png?raw=true">

<img width="280" src="https://github.com/xplpc/xplpc/blob/main/extras/images/screenshot-flutter-macos.png?raw=true">

<img width="280" src="https://github.com/xplpc/xplpc/blob/main/extras/images/screenshot-flutter-windows.png?raw=true">
