from core import host

# archs
has_windows_x64 = False
has_windows_arm64 = False

if host.get_arch_path() == "arm64":
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
                "conan_profile": "xplpc_windows_profile",
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
                "conan_profile": "xplpc_windows_profile",
            },
        ]
    )
