
# Darwin Toolchain

Toolchain required to cross build to any Darwin platform.

## Setup

This package **REQUIRES** Xcode to be installed.

Install with the command:

```
conan create . xplpc/stable
```

### Platforms

Create a profile for cross building and including this toolchain, example:

**iOS**

```
include(default)

[settings]
os=iOS
os.version=12.0
arch=armv8

[tool_requires]
darwin-toolchain/1.0.0@xplpc/stable
```

**watchOS**


```
include(default)

[settings]
os=watchOS
os.version=5.0
arch=armv8_32

[tool_requires]
darwin-toolchain/1.0.0@xplpc/stable
```

**tvOS**

```
include(default)

[settings]
os=tvOS
os.version=12.0
arch=armv8

[tool_requires]
darwin-toolchain/1.0.0@xplpc/stable
```

**macOS catalyst**

```
include(default)

[settings]
os=Macos
os.subsystem=catalyst
arch=x86_64

[tool_requires]
darwin-toolchain/1.0.0@xplpc/stable
```

## Bitcode support

Bitcode is an option available on iOS and it is **required** on tvOS and watchOS.

It is set by default to `True` on tvOS and watchOS, so you can only set it to `False` for iOS.

Note that it is not defined for macOS.

## Local development

1. Enter on project folder:  
```cd conan/darwin-toolchain```
2. Install:  
```conan create . xplpc/stable```
