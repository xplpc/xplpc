# archs
has_mac_x64 = False
has_mac_arm64 = True

data = []

# macos - x64
if has_mac_x64:
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
            },
        ]
    )

# macos - arm64
if has_mac_arm64:
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
            },
        ]
    )
