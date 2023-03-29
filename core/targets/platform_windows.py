import platform

# archs
has_windows_x64 = False
has_windows_arm64 = False

if platform.processor() == "arm":
    has_windows_arm64 = True
else:
    has_windows_x64 = True

data = []

# windows - x64
if has_windows_x64:
    data.extend(
        [
            {
                "arch": "x86_64",
                "conan_arch": "x86_64",
                "conan_profile": "nativium_windows_profile",
            },
        ]
    )

# windows - arm64
if has_windows_arm64:
    data.extend(
        [
            {
                "arch": "arm64",
                "conan_arch": "armv8",
                "conan_profile": "nativium_windows_profile",
            },
        ]
    )
