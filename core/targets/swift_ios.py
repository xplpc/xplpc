# archs
has_ios_arm32 = False
has_ios_arm64 = True
has_ios_simulator_x64 = True
has_ios_simulator_arm64 = True
has_tvos_arm64 = True
has_tvos_simulator_x64 = True
has_tvos_simulator_arm64 = True
has_watchos_arm32 = True
has_watchos_arm64 = True
has_watchos_simulator_x64 = True
has_watchos_simulator_arm64 = False
has_mac_catalyst_x64 = True
has_mac_catalyst_arm64 = True

data = []

# ios - arm32
if has_ios_arm32:
    data.extend(
        [
            {
                "arch": "armv7",
                "conan_arch": "armv7",
                "conan_profile": "xplpc_ios_profile",
                "deployment_target": "12.0",
                "supported_platform": "iPhoneOS",
                "sdk": "iphoneos",
                "group": "ios",
                "platform": "OS",
                "sdk_version": "11.0",
                "enable_arc": True,
                "enable_bitcode": False,
                "enable_visibility": False,
            },
        ]
    )

# ios - arm64
if has_ios_arm64:
    data.extend(
        [
            {
                "arch": "arm64",
                "conan_arch": "armv8",
                "conan_profile": "xplpc_ios_profile",
                "deployment_target": "12.0",
                "supported_platform": "iPhoneOS",
                "sdk": "iphoneos",
                "group": "ios",
                "platform": "OS64",
                "sdk_version": "11.0",
                "enable_arc": True,
                "enable_bitcode": False,
                "enable_visibility": False,
            },
        ]
    )

# ios simulator - x64
if has_ios_simulator_x64:
    data.extend(
        [
            {
                "arch": "x86_64",
                "conan_arch": "x86_64",
                "conan_profile": "xplpc_ios_profile",
                "deployment_target": "12.0",
                "supported_platform": "iPhoneSimulator",
                "sdk": "iphonesimulator",
                "group": "ios-simulator",
                "platform": "SIMULATOR64",
                "sdk_version": "11.0",
                "enable_arc": True,
                "enable_bitcode": False,
                "enable_visibility": False,
            },
        ]
    )

# ios simulator - arm64
if has_ios_simulator_arm64:
    data.extend(
        [
            {
                "arch": "arm64",
                "conan_arch": "armv8",
                "conan_profile": "xplpc_ios_profile",
                "deployment_target": "12.0",
                "supported_platform": "iPhoneSimulator",
                "sdk": "iphonesimulator",
                "group": "ios-simulator",
                "platform": "SIMULATORARM64",
                "sdk_version": "11.0",
                "enable_arc": True,
                "enable_bitcode": False,
                "enable_visibility": False,
            },
        ]
    )

# tvos - arm64
if has_tvos_arm64:
    data.extend(
        [
            {
                "arch": "arm64",
                "conan_arch": "armv8",
                "conan_profile": "xplpc_tvos_profile",
                "deployment_target": "12.0",
                "supported_platform": "AppleTVOS",
                "sdk": "appletvos",
                "group": "tvos",
                "platform": "TVOS",
                "sdk_version": "11.0",
                "enable_arc": True,
                "enable_bitcode": False,
                "enable_visibility": False,
            },
        ]
    )

# tvos simulator - x64
if has_tvos_simulator_x64:
    data.extend(
        [
            {
                "arch": "x86_64",
                "conan_arch": "x86_64",
                "conan_profile": "xplpc_tvos_profile",
                "deployment_target": "12.0",
                "supported_platform": "AppleTVSimulator",
                "sdk": "appletvsimulator",
                "group": "tvos-simulator",
                "platform": "SIMULATOR_TVOS",
                "sdk_version": "11.0",
                "enable_arc": True,
                "enable_bitcode": False,
                "enable_visibility": False,
            },
        ]
    )

# tvos simulator - arm64
if has_tvos_simulator_arm64:
    data.extend(
        [
            {
                "arch": "arm64",
                "conan_arch": "armv8",
                "conan_profile": "xplpc_tvos_profile",
                "deployment_target": "12.0",
                "supported_platform": "AppleTVSimulator",
                "sdk": "appletvsimulator",
                "group": "tvos-simulator",
                "platform": "SIMULATORARM64_TVOS",
                "sdk_version": "11.0",
                "enable_arc": True,
                "enable_bitcode": False,
                "enable_visibility": False,
            },
        ]
    )

# watchos - arm32
if has_watchos_arm32:
    data.extend(
        [
            {
                "arch": "armv7k",
                "conan_arch": "armv7k",
                "conan_profile": "xplpc_watchos_profile",
                "deployment_target": "5.0",
                "supported_platform": "WatchOS",
                "sdk": "watchos",
                "group": "watchos",
                "platform": "WATCHOS",
                "sdk_version": "11.0",
                "enable_arc": True,
                "enable_bitcode": False,
                "enable_visibility": False,
            },
        ]
    )

# watchos - arm64
if has_watchos_arm64:
    data.extend(
        [
            {
                "arch": "arm64_32",
                "conan_arch": "armv8_32",
                "conan_profile": "xplpc_watchos_profile",
                "deployment_target": "5.0",
                "supported_platform": "WatchOS",
                "sdk": "watchos",
                "group": "watchos",
                "platform": "WATCHOS",
                "sdk_version": "11.0",
                "enable_arc": True,
                "enable_bitcode": False,
                "enable_visibility": False,
            },
        ]
    )

# watchos simulator - x64
if has_watchos_simulator_x64:
    data.extend(
        [
            {
                "arch": "x86_64",
                "conan_arch": "x86_64",
                "conan_profile": "xplpc_watchos_profile",
                "deployment_target": "5.0",
                "supported_platform": "WatchSimulator",
                "sdk": "watchsimulator",
                "group": "watchos-simulator",
                "platform": "SIMULATOR_WATCHOS",
                "sdk_version": "11.0",
                "enable_arc": True,
                "enable_bitcode": False,
                "enable_visibility": False,
            },
        ]
    )

# watchos simulator - arm64
if has_watchos_simulator_arm64:
    data.extend(
        [
            {
                "arch": "arm64",
                "conan_arch": "armv8",
                "conan_profile": "xplpc_watchos_profile",
                "deployment_target": "5.0",
                "supported_platform": "WatchSimulator",
                "sdk": "watchsimulator",
                "group": "watchos-simulator",
                "platform": "SIMULATOR_WATCHOS",
                "sdk_version": "11.0",
                "enable_arc": True,
                "enable_bitcode": False,
                "enable_visibility": False,
            },
        ]
    )

# mac catalyst
if has_mac_catalyst_x64:
    data.extend(
        [
            {
                "arch": "x86_64",
                "conan_arch": "x86_64",
                "conan_profile": "xplpc_catalyst_profile",
                "deployment_target": "13.1",
                "supported_platform": "MacOSX",
                "sdk": "macosx",
                "group": "mac-catalyst",
                "subsystem_ios_version": "13.1",
                "platform": "MAC_CATALYST",
                "sdk_version": "11.0",
                "enable_arc": True,
                "enable_bitcode": False,
                "enable_visibility": False,
            },
        ]
    )

# mac catalyst arm64
if has_mac_catalyst_arm64:
    data.extend(
        [
            {
                "arch": "arm64",
                "conan_arch": "armv8",
                "conan_profile": "xplpc_catalyst_profile",
                "deployment_target": "13.1",
                "supported_platform": "MacOSX",
                "sdk": "macosx",
                "group": "mac-catalyst",
                "subsystem_ios_version": "13.1",
                "platform": "MAC_CATALYST_ARM64",
                "sdk_version": "11.0",
                "enable_arc": True,
                "enable_bitcode": False,
                "enable_visibility": False,
            },
        ]
    )
