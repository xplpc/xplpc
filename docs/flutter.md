# Flutter

Flutter is a versatile platform that supports a wide range of operating systems, including iOS, Android, macOS, Linux and Windows. 

To ensure seamless integration with these diverse environments, developers are required to build XPLPC with interface support that allows for direct C function calls from Dart.

This interface support is essential for ensuring that your Flutter app can take full advantage of the powerful features offered by C functions, such as enhanced performance and optimized memory usage. 

With XPLPC and interface support, developers can easily create cross-platform applications that run seamlessly on a range of operating systems and deliver a consistent user experience across all devices.

Whether you're developing for mobile or desktop, Flutter with XPLPC provides a powerful and flexible framework that empowers developers to create robust and performant applications.

## How to build the native library for iOS

Execute on terminal:

    python3 xplpc.py flutter-build-xcframework

## How to build the native library for macOS

Execute on terminal:

    python3 xplpc.py flutter-build-xcframework-macos

## Sample project

Before using the sample, don't forget to build the native library for your platform first.

You can see the sample project in directory `flutter/plugin/example`.
