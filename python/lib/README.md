# XPLPC

Cross Platform Lite Procedure Call for Python.

XPLPC lets Python call a procedure implemented in C++, Kotlin, Swift, Dart or JavaScript, and lets any of them call one implemented in Python. The data is serialized and carried through device memory rather than over a network protocol, so there is no server and no port.

This package is the Python side of the bridge. It reaches the C++ core through the C ABI with `ctypes`, and it carries the native library for the platform it was built for.

## Getting started

```python
from xplpc.core.config import Config
from xplpc.core.xplpc import XPLPC
from xplpc.serializer.json_serializer import JsonSerializer

XPLPC().initialize(Config(JsonSerializer()))

request = Request(
    "sample.login",
    [
        Param("username", "paulo"),
        Param("password", "123456"),
        Param("remember", True),
    ],
)

print(Client.call_sync(request, str))
```

A mapping this side owns is registered by the name every other language reaches it through:

```python
MappingList().add("platform.battery.level", MappingItem(battery_level))
```

## Documentation

The guide for this platform covers building the native library, installing the package, the three ways of making a call, writing a mapping that answers later, carrying a large buffer with `DataView`, and how to raise the log level.

<https://github.com/xplpc/xplpc/blob/main/docs/python.md>

## Supported platforms

Linux, macOS and Windows.

## License

MIT. See [LICENSE](https://github.com/xplpc/xplpc/blob/main/LICENSE).
