# archs
has_arm32 = True
has_arm64 = True
has_x32 = True
has_x64 = True

data = []

# arm32
if has_arm32:
    data.extend(
        [
            {
                "arch": "armeabi-v7a",
                "conan_arch": "armv7",
                "conan_profile": "xplpc_android_profile",
                "api_level": 19,
            },
        ]
    )

# arm64
if has_arm64:
    data.extend(
        [
            {
                "arch": "arm64-v8a",
                "conan_arch": "armv8",
                "conan_profile": "xplpc_android_profile",
                "api_level": 21,
            },
        ]
    )

# x32
if has_x32:
    data.extend(
        [
            {
                "arch": "x86",
                "conan_arch": "x86",
                "conan_profile": "xplpc_android_profile",
                "api_level": 19,
            },
        ]
    )

# x64
if has_x64:
    data.extend(
        [
            {
                "arch": "x86_64",
                "conan_arch": "x86_64",
                "conan_profile": "xplpc_android_profile",
                "api_level": 21,
            },
        ]
    )
