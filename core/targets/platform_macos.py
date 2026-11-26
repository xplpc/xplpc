from core import host

# archs
has_macos_x64 = False
has_macos_arm64 = False

if host.get_arch_path() == "arm64":
    has_macos_arm64 = True
else:
    has_macos_x64 = True

data = []

# macos - x64
if has_macos_x64:
    data.extend(
        [
            {
                "arch": "x86_64",
                "conan_arch": "x86_64",
                "conan_profile": "xplpc_macos_profile",
                "deployment_target": "12.0",
                "enable_arc": True,
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
                "deployment_target": "12.0",
                "enable_arc": True,
                "enable_visibility": False,
            },
        ]
    )
