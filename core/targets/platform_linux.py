from core import host

# archs
has_linux_x64 = False
has_linux_arm64 = False

if host.get_arch_path() == "arm64":
    has_linux_arm64 = True
else:
    has_linux_x64 = True

data = []

# linux - x64
if has_linux_x64:
    data.extend(
        [
            {
                "arch": "x86_64",
                "conan_arch": "x86_64",
                "conan_profile": "xplpc_linux_profile",
            },
        ]
    )

# linux - arm64
if has_linux_arm64:
    data.extend(
        [
            {
                "arch": "arm64",
                "conan_arch": "armv8",
                "conan_profile": "xplpc_linux_profile",
            },
        ]
    )
