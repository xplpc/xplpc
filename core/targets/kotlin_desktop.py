import platform

from pygemstones.system import platform as p

from xplpc import util

# general
data = []

# archs
has_windows_x64 = False
has_windows_arm64 = False

has_linux_x64 = False
has_linux_arm64 = False

has_macos_x64 = False
has_macos_arm64 = False

# check arch
arch = util.get_arch_path()

if p.is_windows():
    if platform.processor() == "arm":
        has_windows_arm64 = True
    else:
        has_windows_x64 = True
elif p.is_linux():
    if platform.processor() == "arm":
        has_linux_arm64 = True
    else:
        has_linux_x64 = True
elif p.is_macos():
    if platform.processor() == "arm":
        has_macos_arm64 = True
    else:
        has_macos_x64 = True

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
# macos - x64
if has_macos_x64:
    data.extend(
        [
            {
                "arch": "x86_64",
                "conan_arch": "x86_64",
                "conan_profile": "xplpc_macos_profile",
                "deployment_target": "10.15",
                "sdk": "macosx",
                "platform": "MAC",
                "sdk_version": "11.0",
                "group": "macos",
                "enable_arc": True,
                "enable_bitcode": False,
                "enable_visibility": False,
            },
        ]
    )

# macos - arm64
if has_macos_arm64:
    data.extend(
        [
            {
                "arch": "arm64",
                "conan_arch": "armv8",
                "conan_profile": "xplpc_macos_profile",
                "deployment_target": "11.0",
                "sdk": "macosx",
                "platform": "MAC_ARM64",
                "sdk_version": "11.0",
                "group": "macos",
                "enable_arc": True,
                "enable_bitcode": False,
                "enable_visibility": False,
            },
        ]
    )
