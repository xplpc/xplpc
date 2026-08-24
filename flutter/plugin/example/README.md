# xplpc\_example

Demonstrates how to use the XPLPC plugin.

The application calls `sample.login`, which is implemented in C++, and `platform.battery.level`, which is implemented in Dart and registered with `MappingList`. The second one still crosses the bridge, since the native side is what routes a call to whoever owns the name, so one screen exercises both directions.

Building it needs the native library for the platform you are running on, which the guide for this platform covers along with the Reflectable setup the serializer depends on.

<https://github.com/xplpc/xplpc/blob/main/docs/flutter.md>
