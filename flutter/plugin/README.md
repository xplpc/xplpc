# XPLPC

Cross Platform Lite Procedure Call for Flutter.

XPLPC lets Dart call a procedure implemented in C++, or in Kotlin on Android and Swift on iOS and macOS, and lets any of them call one implemented in Dart. The data is serialized and carried through device memory rather than over a network protocol, so there is no server and no port.

This package is the Dart side of the bridge. It reaches the C++ core through `dart:ffi`, which is why it is an FFI plugin and why the native library has to be built for the platform you are targeting.

## Getting started

```dart
XPLPC.instance.initialize(Config(JsonSerializer()));

final request = Request("sample.login", [
  Param("username", "paulo"),
  Param("password", "123456"),
  Param("remember", true),
]);

Client.call<String>(request, (response) {
  print(response);
});
```

A mapping this side owns is registered by the name every other language reaches it through:

```dart
MappingList.instance.add("platform.battery.level", MappingItem(batteryLevel));
```

Reading an answer into a class of your own needs reflectable, since Flutter is compiled without reflection and the serializer fills that class through the `fromJson` constructor it declares. A program that reads only plain values needs nothing more than the plugin.

## Documentation

The guide for this platform covers building the native library for each target, the setup Reflectable needs, the three ways of making a call, which are `call`, `callSync` and `callAsync`, writing a mapping that answers later, carrying a large buffer with `DataView`, and how to raise the log level.

<https://github.com/xplpc/xplpc/blob/main/docs/flutter.md>

## Supported platforms

Android from sdk 24, iOS 15, macOS 12, Linux and Windows, on Flutter 3.50 and Dart 3.14. On iOS and macOS the framework arrives through Swift Package Manager, and through the podspec when a project turned it off.

## License

MIT. See [LICENSE](LICENSE).
